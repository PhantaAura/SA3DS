#include "ccl_wall.h"
#include <math.h>

SA3DSWallResponse sa3ds_ccl_wall_sample(SA1Vec3 sample, float player_radius,
    float player_half_height, SA1Vec3 extents, float *corrected_z){
    if(!corrected_z||!isfinite(sample.z)||player_radius<0||player_half_height<0||
       !(extents.x>0&&extents.y>0&&extents.z>0))return SA3DS_WALL_MISS;

    if(!(sample.x+player_radius>-extents.x&&sample.x-player_radius<extents.x&&
         sample.y+player_half_height>-extents.y&&sample.y-player_half_height<extents.y))
        return SA3DS_WALL_MISS;

    /* 41A580 uses a soft displacement in the 50-unit band beyond the
       front face, and a position write closer to the plane. */
    float depth=sample.z-extents.z;
    if(depth>=0&&depth<50.f){
        *corrected_z=(float)(((double)extents.z+50.0-sample.z)*0.06+sample.z);
        return SA3DS_WALL_SOFT;
    }
    if(depth<0&&sample.z>-.2f){
        *corrected_z=(float)((double)extents.z+0.1);
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
