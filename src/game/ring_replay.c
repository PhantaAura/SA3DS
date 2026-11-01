#include "ring_replay.h"
#include "emerald_coast_collision.h"
#include <math.h>
SA1Input sa3ds_ring_replay_input_mode(const SA3DSRings*r,const SA1PlayerState*p,float camera,unsigned tick,int groups_only){
    SA1Input in={0};
    if(!r||!p||!r->states||!isfinite(camera)||tick<60||(groups_only?r->group_collected_events:r->collected_events)>=3)return in;
    float best=INFINITY;SA1Vec3 target={0};
    SA3CMatrix actor;float center_y=p->pos.y;
    if(groups_only){sa3ds_ec_model_transform(p,&actor);center_y=actor.m[13];}
    for(uint32_t i=0;i<r->count;i++){
        SA3DSRingPlacement row;
        if(r->states[i].collected||!sa3ds_rings_placement(r,i,&row)||row.act!=r->act)continue;
        if(groups_only&&row.group<0)continue;
        unsigned high=((row.group<0?row.type:r->groups[row.group].placement.type)>>12)&7;
        if(high&&(high==1?r->clip_level>=1:r->clip_level>=2))continue;
        float dx=row.position.x-p->pos.x,dz=row.position.z-p->pos.z;
        float dy=groups_only?row.position.y-center_y:0;
        float d2=dx*dx+dz*dz+dy*dy;
        if(d2<best){best=d2;target=row.position;}
    }
    if(!isfinite(best)||best<25)return in;
    float angle=atan2f(target.x-p->pos.x,target.z-p->pos.z)-camera;
    float strength=best<1600?.45f:.7f;
    in.move_x=-sinf(angle)*strength;in.move_z=cosf(angle)*strength;
    return in;
}

SA1Input sa3ds_ring_replay_input(const SA3DSRings*r,const SA1PlayerState*p,float camera,unsigned tick){
    return sa3ds_ring_replay_input_mode(r,p,camera,tick,0);
}
