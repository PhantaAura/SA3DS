#include "ccl_wall.h"
#include <math.h>

SA3DSWallResponse sa3ds_ccl_wall_sample(SA1Vec3 sample, float player_radius,
    float player_half_height, SA1Vec3 extents, float *corrected_z){
    if(!corrected_z||!isfinite(sample.z)||player_radius<0||player_half_height<0||
       !(extents.x>0&&extents.y>0))return SA3DS_WALL_MISS;

    if(!(sample.x+player_radius>-extents.x&&sample.x-player_radius<extents.x&&
         sample.y+player_half_height>-extents.y&&sample.y-player_half_height<extents.y))
        return SA3DS_WALL_MISS;

    /* 41A580 uses a soft displacement in the 50-unit band beyond the
       front face, and a position write closer to the plane. */
    /* 41A583 loads player shape+14 into the saved radius. The wall's
       Z extent only contributes to broad phase, not this one-sided plane. */
    float depth=sample.z-player_radius;
    if(depth>=0&&depth<50.f){
        *corrected_z=(float)(((double)player_radius+50.0-sample.z)*(double).06f+sample.z);
        return SA3DS_WALL_SOFT;
    }
    if(depth<0&&sample.z>-.2f){
        *corrected_z=(float)((double)player_radius+(double).1f);
        return SA3DS_WALL_HARD;
    }
    return SA3DS_WALL_MISS;
}

SA3DSWallResponse sa3ds_ccl_wall_sweep(SA1Vec3 previous, SA1Vec3 current,
    float player_radius, float player_half_height, SA1Vec3 extents, SA1Vec3 *corrected){
    if(!corrected)return SA3DS_WALL_MISS;

    SA1Vec3 delta={current.x-previous.x,current.y-previous.y,current.z-previous.z};
    float distance=(float)sqrt((double)delta.x*delta.x+(double)delta.y*delta.y+
                              (double)delta.z*delta.z);
    if(!isfinite(distance))return SA3DS_WALL_MISS;
    double sample_count=ceil((double)distance*.4f);
    if(sample_count>65536)return SA3DS_WALL_MISS;

    SA1Vec3 step={0};
    if(distance>0){
        step.x=(float)((double)delta.x/distance*2.5);
        step.y=(float)((double)delta.y/distance*2.5);
        step.z=(float)((double)delta.z/distance*2.5);
    }

    /* 41A580 steps by 2.5 units, then substitutes the exact endpoint
       whenever the remaining counter is below one. */
    int remaining=(int)sample_count;
    SA1Vec3 point=previous;
    point.y+=player_half_height;
    do{
        if(--remaining<1){
            point=current;
            point.y+=player_half_height;
        }else{
            point.x+=step.x;
            point.y+=step.y;
            point.z+=step.z;
        }
        float z;
        SA3DSWallResponse response=sa3ds_ccl_wall_sample(point,player_radius,
            player_half_height,extents,&z);
        if(response!=SA3DS_WALL_MISS){
            *corrected=(SA1Vec3){point.x,point.y-player_half_height,z};
            return response;
        }
    }while(remaining>=0);
    return SA3DS_WALL_MISS;
}

static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
float sa3ds_ccl_wall_broad_radius(SA1Vec3 d){
    float radius=(float)sqrt((double)d.x*d.x+(double)d.y*d.y+(double)d.z*d.z);
    return radius<.1f?25.f:(float)((double)radius*(double)1.01f);
}
SA3DSWallResponse sa3ds_ccl_wall_world_sweep(SA1Vec3 previous,SA1Vec3 current,
    SA1Vec3 center,uint16_t yaw,float radius,float half_height,SA1Vec3 dimensions,
    SA3DSWallSine sine,void *context,SA1Vec3 *corrected){
    if(!corrected||!sine||!finite3(previous)||!finite3(current)||!finite3(center)||
        !finite3(dimensions)||!isfinite(radius)||!isfinite(half_height)||radius<0||half_height<0)
        return SA3DS_WALL_MISS;
    float sn=(float)sine(context,yaw),cs=(float)sine(context,(uint16_t)(yaw+0x4000));
    if(!isfinite(sn)||!isfinite(cs))return SA3DS_WALL_MISS;
    SA1Vec3 delta={current.x-previous.x,current.y-previous.y,current.z-previous.z};
    /*41A629 stores the squared sum as float before sqrt.*/
    float squared=(float)(((double)delta.z*delta.z+(double)delta.y*delta.y)+(double)delta.x*delta.x);
    float distance=(float)sqrt((double)squared);
    SA1Vec3 step={0};
    if(distance>0){
        double inverse=1.0/distance;
        step.x=(float)((double)delta.x*inverse*2.5);
        /*Y/Z normalized products have separate float stores at41A658/662.*/
        step.y=(float)((double)(float)(delta.y*inverse)*2.5);
        step.z=(float)((double)(float)(delta.z*inverse)*2.5);
    }else distance=.01f;
    double count=ceil((double)distance*(double).4f);
    if(!isfinite(count)||count>65536)return SA3DS_WALL_MISS;
    int remaining=(int)count;
    SA1Vec3 point=previous;point.y+=half_height;
    do{
        if(--remaining<1){point=current;point.y+=half_height;}
        else{point.x+=step.x;point.y+=step.y;point.z+=step.z;}
        SA1Vec3 relative={point.x-center.x,point.y-center.y,point.z-center.z};
        SA1Vec3 local={(float)((double)relative.x*cs-(double)relative.z*sn),relative.y,
                       (float)((double)relative.z*cs+(double)relative.x*sn)};
        float z;SA3DSWallResponse result=sa3ds_ccl_wall_sample(local,radius,half_height,dimensions,&z);
        if(result!=SA3DS_WALL_MISS){
            float x=(float)((double)local.x*cs+(double)z*sn);
            float world_z=(float)((double)z*cs-(double)local.x*sn);
            *corrected=(SA1Vec3){x+center.x,current.y,world_z+center.z};
            return result;
        }
    }while(remaining>=0);
    return SA3DS_WALL_MISS;
}
