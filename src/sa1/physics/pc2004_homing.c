#include "pc2004_homing.h"
#include <math.h>
#include <string.h>
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static int16_t signed16(uint16_t value){int16_t s;memcpy(&s,&value,2);return s;}
static float task_distance(SA1Vec3 a,SA1Vec3 b){
    double x=(double)a.x-b.x,y=(double)a.y-b.y,z=(double)a.z-b.z;
    return (float)(x*x+y*y+z*z);
}
unsigned sa1_pc2004_homing_angle_distance(uint16_t a,uint16_t b){
    int delta=signed16((uint16_t)(b-a));return (unsigned)(delta<0?-delta:delta);
}
static uint16_t approach(uint16_t a,uint16_t b,unsigned limit){
    int delta=signed16((uint16_t)(b-a));
    if(delta>(int)limit)return (uint16_t)(a+limit);
    if(delta<-(int)limit)return (uint16_t)(a-limit);
    return b;
}
void sa1_pc2004_homing_reset(SA1PC2004HomingList*l){if(l)l->count=0;}
int sa1_pc2004_homing_collect(SA1PC2004HomingList*l,int player,unsigned category,
    unsigned flags,uint8_t task_id,uint32_t id,SA1Vec3 origin,SA1Vec3 target){
    if(!l||l->count>SA1_PC2004_HOMING_CAPACITY||!id||!finite3(origin)||!finite3(target))return -1;
    if((player!=0&&player!=1)||category!=0||!(flags&0x40u)||task_id==11)return 0;
    float distance=task_distance(origin,target);
    if(!(distance<10000.f)||l->count==SA1_PC2004_HOMING_CAPACITY)return 0;
    l->entries[l->count++]=(SA1PC2004HomingCandidate){id,distance};return 1;
}
static int pose(const SA1PC2004HomingInput*in,uint32_t id,SA1PC2004HomingPose*out){
    return in->resolve&&in->resolve(in->resolve_context,id,out)&&finite3(out->task_origin)&&finite3(out->first_center);
}
static SA1Vec3 delta_center(SA1PC2004HomingPose p,SA1Vec3 player){
    SA1Vec3 center=p.first_center;
    if(!(p.first_shape_flags&0x20u)){
        center.x+=p.task_origin.x;center.y+=p.task_origin.y;center.z+=p.task_origin.z;
    }
    return (SA1Vec3){center.x-player.x,center.y-player.y,center.z-player.z};
}
int sa1_pc2004_homing_step(const SA1PC2004AirAttack*s,const SA1PC2004HomingInput*in,SA1PC2004HomingResult*out){
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    if(!s||!in||!in->list||s->action!=14||in->list->count>SA1_PC2004_HOMING_CAPACITY||
       !finite3(in->player_task_origin)||!isfinite(in->local_speed.forward)||
       !isfinite(in->local_speed.normal)||!isfinite(in->local_speed.side))return 0;
    SA1PC2004HomingResult r={.next=*s,.task_yaw=in->task_yaw,.local_speed=in->local_speed};
    if((s->timer>0||!in->list->count)&&!in->partner_id){
        r.mode=SA1_PC2004_HOMING_DECAY_ACCEL;
        r.next.timer=signed16((uint16_t)((uint16_t)s->timer+1u));
        r.local_speed.forward*=.9f;*out=r;return 1;
    }
    if(!in->math.bearing||!in->math.sine)return 0;
    float best=10000.f;uint32_t selected=0;
    if(in->partner_id){
        SA1PC2004HomingPose p;if(!pose(in,in->partner_id,&p))return 0;
        SA1Vec3 d={p.task_origin.x-in->player_task_origin.x,p.task_origin.y-in->player_task_origin.y,p.task_origin.z-in->player_task_origin.z};
        float distance=task_distance(p.task_origin,in->player_task_origin);
        uint16_t yaw=in->math.bearing(in->math.context,d.z,d.x);
        if(sa1_pc2004_homing_angle_distance(in->task_yaw,yaw)<=0x5000u&&distance<10000.f){best=distance;selected=in->partner_id;}
    }
    for(unsigned i=0;i<in->list->count;i++){
        const SA1PC2004HomingCandidate *c=&in->list->entries[i];
        if(!c->actor_id||!isfinite(c->task_distance_squared)||c->task_distance_squared<0)return 0;
        SA1PC2004HomingPose p;if(!pose(in,c->actor_id,&p))return 0;
        SA1Vec3 d=delta_center(p,in->player_task_origin);
        uint16_t yaw=in->math.bearing(in->math.context,d.z,d.x);
        if((in->special_player||sa1_pc2004_homing_angle_distance(in->task_yaw,yaw)<=0x5000u)&&
           c->task_distance_squared<best){best=c->task_distance_squared;selected=c->actor_id;}
    }
    if(!selected){r.next.timer=1;r.mode=SA1_PC2004_HOMING_ACCEL_ONLY;*out=r;return 1;}
    SA1PC2004HomingPose target;if(!pose(in,selected,&target))return 0;
    SA1Vec3 d=delta_center(target,in->player_task_origin);
    r.task_yaw=approach(in->task_yaw,in->math.bearing(in->math.context,d.z,d.x),in->special_player?0x1000u:0x800u);
    if(d.y>0&&s->timer<0){d.y=0;r.next.timer=1;}
    float length=(float)sqrt((double)d.x*d.x+(double)d.y*d.y+(double)d.z*d.z);
    float vertical=0;
    if(length!=0){float inverse=(float)(1.0/(double)length);vertical=d.y*inverse;}
    double horizontal2=1.0-(double)vertical*vertical;
    if(horizontal2<0)return 0;
    float horizontal=(float)sqrt(horizontal2);
    double cosine=in->math.sine(in->math.context,(uint16_t)(r.task_yaw+0x4000));
    double sine=in->math.sine(in->math.context,r.task_yaw);
    if(!isfinite(cosine)||!isfinite(sine))return 0;
    float x=(float)(cosine*horizontal),z=(float)(sine*horizontal);
    float speed=in->special_player?10.f:5.f;
    if(s->age>180){
        if(!in->math.random15)return 0;
        uint16_t random=in->math.random15(in->math.context);if(random>32767)return 0;
        speed=(float)(((double)random*(double)(1.f/32768.f)*(double).1f+(double).7f)*(double)speed);
    }
    r.world_velocity=(SA1Vec3){x*speed,vertical*speed,z*speed};
    if(!finite3(r.world_velocity))return 0;
    if(r.next.timer<0)r.next.timer=0;
    r.mode=SA1_PC2004_HOMING_TRACK;r.selected_id=selected;*out=r;return 1;
}
