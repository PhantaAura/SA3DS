#include "ring_scatter.h"
#include <math.h>
#include <string.h>
static int finite3(SA1Vec3 v){return isfinite(v.x)&&isfinite(v.y)&&isfinite(v.z);}
static void draw(SA3DSRingScatter*a,const SA3DSRingScatterInput*i,SA3DSRingScatterResult*r){
    r->normal_draw=1;if(!i->paused)a->spin=(float)((double)a->spin+5.);
}
static void collect(SA3DSRingScatter*a,const SA3DSRingScatterInput*i,SA3DSRingScatterResult*r,int secondary_phase){
    r->clear_hit=r->reset_pickup_effect=1;
    a->spin=0;a->radial_x=-2;a->radial_z=-4;a->action=4;
    a->position.y=(float)((double)a->position.y+3.440000057220459);
    if(!i->primary_dead&&(secondary_phase||i->hit_is_primary_player))r->award_ring=r->sound7=1;
    if(secondary_phase)r->display_callback=3;
}
int sa3ds_ring_scatter_tick(SA3DSRingScatter*a,int32_t*phase,const SA3DSRingScatterInput*i,
    const SA3DSRingScatterServices*v,SA3DSRingScatterResult*r){
    if(!a||!phase||!i||!v||!r||!finite3(a->position)||!isfinite(a->spin)||
       !isfinite(a->scale_or_floor)||!isfinite(a->launch_y)||!isfinite(a->decay_age)||
       !isfinite(a->radial_x)||!isfinite(a->radial_z)||!isfinite(a->floor_phase)||!isfinite(a->cached_floor)||
       (a->action==4&&(fabs((double)a->spin)>2147483647.||fabs((double)a->radial_x)>2147483647.||fabs((double)a->radial_z)>2147483647.))||a->action>4)return 0;
    memset(r,0,sizeof(*r));for(unsigned n=0;n<3;n++)r->particle_frames[n]=-1;
    if(a->action==0){
        if(!v->random15||*phase<0||*phase>8)return 0;
        uint16_t random=v->random15(v->context);if(random>32767)return 0;
        a->cached_floor=(float)((double)a->position.y-100.);a->floor_phase=(float)*phase;
        if(++*phase>8)*phase=0;
        a->position.y=(float)((double)a->position.y+10.);a->action=1;
        a->radial_x=a->radial_z=.3f;a->launch_y=4.6f;a->scale_or_floor=a->decay_age=1;
        a->spin=(float)(int32_t)trunc((double)random*(double)(1.f/32768.f)*(double)360.f*65536.*(1./360.));
        a->age=0;a->collection_delay=0;a->collision_id=11;r->initialize_collision=1;return 1;
    }
    if(a->action==3){r->destroy=1;return 1;}
    if(a->action==4){
        if(!i->paused){a->spin=(float)((double)a->spin+.8f);a->radial_x=(float)((double)a->radial_x+.8f);a->radial_z=(float)((double)a->radial_z+.8f);}
        if(!i->render_suppressed){
            r->pickup_effect=1;float frames[3]={a->spin,a->radial_x,a->radial_z};
            for(unsigned n=0;n<3;n++){int frame=(int)truncf(frames[n]);if(frame>=0&&frame<7)r->particle_frames[n]=frame;}
            if((int)truncf(a->radial_z)>7)a->action=3;
        }
        return 1;
    }
    if(i->hit){collect(a,i,r,a->action==2);return 1;}
    if(a->action==2){
        r->enqueue_collision=1;r->collision_position=(SA1Vec3){
            (float)((double)a->position.x+a->radial_x),
            (float)((double)a->position.y+3.440000057220459),
            (float)((double)a->position.z+a->radial_z)};
        a->age=(uint16_t)(a->age+1);
        if(i->global_frame&2)draw(a,i,r);
        if(a->age>=100)a->action=3;
        r->display_callback=2;return 1;
    }
    if(!v->floor||!v->sine||!v->tilt||!v->atan_bams)return 0;
    a->floor_phase=(float)((double)a->floor_phase-1.);
    if(a->floor_phase<0){
        a->floor_phase=8;SA1Vec3 query=a->position;query.y=(float)((double)query.y+6.880000114440918);
        double floor=v->floor(v->context,query,a->angles);if(!isfinite(floor))return 0;
        a->cached_floor=(float)floor;if(floor==-1000000.)a->cached_floor=(float)((double)a->position.y-100.);
    }
    a->age=(uint16_t)(a->age+1);
    double sx=v->sine(v->context,(uint16_t)a->angles[1]);
    double cz=v->sine(v->context,(uint16_t)(a->angles[1]+0x4000));
    if(!isfinite(sx)||!isfinite(cz))return 0;
    a->radial_x=(float)(sx*(double).3f);a->radial_z=(float)(cz*(double).3f);
    if(a->scale_or_floor>=.2f){
        a->position.x=(float)((double)a->radial_x*a->scale_or_floor+a->position.x);
        a->position.z=(float)((double)a->radial_z*a->scale_or_floor+a->position.z);
        if(a->launch_y<0)a->position.y=a->cached_floor;
        else a->position.y=(float)(((double)a->launch_y-(double)a->age*(double)(1.f/3.f))*(double)(1.f/3.f)+a->position.y);
        if(a->position.y<=a->cached_floor){a->age=0;a->scale_or_floor=(float)((double)a->scale_or_floor-.04f);a->launch_y=(float)((double)a->launch_y-.28f);}
        SA1Vec3 up=v->tilt(v->context,a->angles,(SA1Vec3){0,1,0});if(!finite3(up))return 0;
        if(up.y<.7f){
            SA1Vec3 direction=v->tilt(v->context,a->angles,(SA1Vec3){a->radial_x,1,a->radial_z});if(!finite3(direction))return 0;
            a->angles[1]=v->atan_bams(v->context,direction.x,direction.z);
        }
        a->collection_delay=(uint8_t)(a->collection_delay+1);int8_t delay;memcpy(&delay,&a->collection_delay,1);
        if(delay>46){a->collection_delay=47;r->enqueue_collision=1;r->collision_position=a->position;r->collision_position.y=(float)((double)a->position.y+3.440000057220459);}
        draw(a,i,r);r->display_callback=1;return 1;
    }
    /*Every source low-scale branch transitions to2 THIS tick. The2.4/13
     * tests choose its drift; they are not an extra multi-tick phase.*/
    a->decay_age=(float)((double)a->decay_age+.08f);
    SA1Vec3 drift={0};
    if(a->decay_age<2.4f){
        double scale=(double).3f/a->decay_age;
        a->position.x=(float)(scale*a->radial_x+a->position.x);a->position.z=(float)(scale*a->radial_z+a->position.z);
    }else if(a->decay_age>=13.f){drift=v->tilt(v->context,a->angles,(SA1Vec3){0,6.880000114440918f,0});if(!finite3(drift))return 0;}
    a->action=2;a->radial_x=drift.x;a->radial_z=drift.z;a->scale_or_floor=a->cached_floor;return 1;
}
