#include "saru_pose.h"
#include <math.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return p[0]|((uint16_t)p[1]<<8);}
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static const uint8_t *section(const SA3CCharacter*c,unsigned i){return c->data+c->sections[i].offset;}
static float weighted(double a,double b,double t,int angle){
    volatile double left=a*(1.-t),right=b*t;
    double value=left+right;
    return angle?(float)trunc(value):(float)value;
}
static void keys(const SA3CCharacter*c,uint32_t first,uint16_t count,float frame,int angle,float out[3]){
    if(!count)return;
    const uint8_t*k=section(c,9)+(size_t)first*16;
    unsigned previous=0;
    for(unsigned i=1;i<count;i++){if(r32(k+i*16)>(uint32_t)frame)break;previous=i;}
    unsigned next=(previous+1)%count;
    const uint8_t*a=k+previous*16,*b=k+next*16;
    uint32_t af=r32(a),bf=r32(b);double t=((double)frame-af)/(bf>af?(double)(float)(bf-af):1.);
    const float to_bams=65536.f/6.28318530717958647692f;
    for(unsigned i=0;i<3;i++){
        double av=rf(a+4+i*4),bv=rf(b+4+i*4);
        if(angle){av=roundf((float)av*to_bams);bv=roundf((float)bv*to_bams);}
        out[i]=weighted(av,bv,t,angle);
    }
}
static int sample_motion(const SA3CCharacter*c,unsigned action,float frame,SA3CTransform pose[38],uint16_t*root){
    const uint8_t*a=section(c,7)+action*c->sections[7].stride;
    uint16_t frames=r16(a+4);*root=r16(a+2);
    if(!frames||*root>=38||r16(a+6)!=2||r32(a+16)!=3)return 0;
    frame=fmodf(frame,(float)frames);
    if(sa3c_pose_bind(c,pose,38)!=SA3C_POSE_OK)return 0;
    /* Blend temporary rotations are integer BAMS, including bind fallbacks. */
    const float to_bams=65536.f/6.28318530717958647692f;
    for(unsigned i=0;i<38;i++){
        pose[i].rotation.x=roundf(pose[i].rotation.x*to_bams);
        pose[i].rotation.y=roundf(pose[i].rotation.y*to_bams);
        pose[i].rotation.z=roundf(pose[i].rotation.z*to_bams);
    }
    uint32_t first=r32(a+8),count=r32(a+12);
    for(uint32_t i=0;i<count;i++){
        const uint8_t*ch=section(c,8)+(size_t)(first+i)*c->sections[8].stride;
        uint16_t node=r16(ch),flags=r16(ch+2);if(node>=38||flags&~3u)return 0;
        float p[3]={pose[node].position.x,pose[node].position.y,pose[node].position.z};
        float r[3]={pose[node].rotation.x,pose[node].rotation.y,pose[node].rotation.z};
        if(flags&1)keys(c,r32(ch+4),r16(ch+8),frame,0,p);
        if(flags&2)keys(c,r32(ch+12),r16(ch+10),frame,1,r);
        pose[node].position=(SA3CVec3){p[0],p[1],p[2]};pose[node].rotation=(SA3CVec3){r[0],r[1],r[2]};
    }
    return 1;
}
SA3CPoseResult sa3ds_saru_pose_sample(const SA3CCharacter*c,const SA3DSSaruAnimation*s,SA3CTransform local[38],uint16_t*root){
    if(!c||!s||!local||!root||!c->data||!isfinite(s->current_frame)||s->current_frame<0||
       !isfinite(s->pending_frame)||s->pending_frame<0||!isfinite(s->blend_weight)||
       s->blend_weight<0||s->blend_weight>1||s->current_action>=6||s->pending_action>=6)return SA3C_POSE_ERR_ARGUMENT;
    if(sa3c_character_validate_references(c)!=SA3C_OK||c->sections[0].count!=38||c->sections[7].count!=6)return SA3C_POSE_ERR_RANGE;
    /* Current owner rig has no SKIP or mixed rotation order; don't claim those profiles. */
    for(unsigned i=0;i<38;i++)if(r32(section(c,0)+i*c->sections[0].stride+8)&0x60)return SA3C_POSE_ERR_UNSUPPORTED_INTERPOLATION;
    if(!(s->enemy_flags&0x200))return sa3c_pose_sample_action(c,s->current_action,s->current_frame,1,local,38,root);
    SA3CTransform other[38];uint16_t destination;
    if(!sample_motion(c,s->current_action,s->current_frame,local,root)||
       !sample_motion(c,s->pending_action,s->pending_frame,other,&destination))return SA3C_POSE_ERR_UNSUPPORTED_INTERPOLATION;
    if(*root!=destination)return SA3C_POSE_ERR_HIERARCHY;
    const float radians=6.28318530717958647692f/65536.f;
    for(unsigned i=0;i<38;i++){
        SA3CTransform*a=&local[i],*b=&other[i];double t=s->blend_weight;
        a->position=(SA3CVec3){weighted(a->position.x,b->position.x,t,0),weighted(a->position.y,b->position.y,t,0),weighted(a->position.z,b->position.z,t,0)};
        a->rotation=(SA3CVec3){weighted(a->rotation.x,b->rotation.x,t,1)*radians,weighted(a->rotation.y,b->rotation.y,t,1)*radians,weighted(a->rotation.z,b->rotation.z,t,1)*radians};
        a->scale=(SA3CVec3){weighted(a->scale.x,b->scale.x,t,0),weighted(a->scale.y,b->scale.y,t,0),weighted(a->scale.z,b->scale.z,t,0)};
    }
    return SA3C_POSE_OK;
}
