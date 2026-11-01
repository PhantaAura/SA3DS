#include "ring_groups.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
static uint32_t r32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static double sample(const SA3DSRings*r,unsigned i){
    i&=4095;
    if(i<1024)return rf(r->quarter_lut+i*4);
    if(i<2048)return rf(r->quarter_lut+(2048-i)*4);
    if(i<3072)return -rf(r->quarter_lut+(i-2048)*4);
    return -rf(r->quarter_lut+(4096-i)*4);
}
double sa3ds_ring_group_sin(const SA3DSRings*r,uint16_t angle){
    if(!r||!r->quarter_lut)return 0;
    unsigned index=angle>>4,frac=angle&15;double a=sample(r,index);
    return frac?a+(sample(r,index+1)-a)*(frac*0.0625):a;
}
static void rotate(float m[9],int axis,float s,float c){
    int a=axis==0?1:0,b=axis==2?1:2;
    for(int row=0;row<3;row++){
        float x=m[a*3+row],y=m[b*3+row];
        if(axis==1){m[a*3+row]=(float)((double)x*c-(double)y*s);m[b*3+row]=(float)((double)y*c+(double)x*s);}
        else {m[a*3+row]=(float)((double)x*c+(double)y*s);m[b*3+row]=(float)((double)y*c-(double)x*s);}
    }
}
static SA1Vec3 child_position(const SA3DSRings*r,const SA3DSRingPlacement*p,unsigned index){
    float x=0,z;float count=p->parameters[0],spacing=p->parameters[1];
    if((int)p->parameters[2]==0){
        z=(float)(index&1?ceil(index*.5)*(double)spacing:index*(double)spacing*-.5);
    }else{
        double angle=((double)(index*360)/(double)count)*65536.0*(1.0/360.0);
        uint16_t bams=(uint16_t)(uint32_t)angle;
        x=(float)(sa3ds_ring_group_sin(r,bams)*spacing);
        z=(float)(sa3ds_ring_group_sin(r,(uint16_t)(bams+0x4000))*spacing);
    }
    float matrix[9]={1,0,0,0,1,0,0,0,1};
    const int order[3]={2,0,1};
    for(int i=0;i<3;i++){
        int axis=order[i];uint16_t a=(uint16_t)p->angles[axis];
        if(a)rotate(matrix,axis,(float)sa3ds_ring_group_sin(r,a),
            (float)sa3ds_ring_group_sin(r,(uint16_t)(a+0x4000)));
    }
    float transformed[3];for(int i=0;i<3;i++)transformed[i]=(float)((double)matrix[i]*x+(double)matrix[i+6]*z);
    return (SA1Vec3){transformed[0]+p->position.x,transformed[1]+p->position.y,transformed[2]+p->position.z};
}
static void placement(const uint8_t*p,SA3DSRingPlacement*out){
    memset(out,0,sizeof(*out));out->act=r32(p);out->source_row=r32(p+4);out->type=r16(p+8);out->group=-1;
    for(int j=0;j<3;j++){uint16_t a=r16(p+10+j*2);memcpy(&out->angles[j],&a,2);}
    out->position=(SA1Vec3){rf(p+16),rf(p+20),rf(p+24)};
    for(int j=0;j<3;j++)out->parameters[j]=rf(p+28+j*4);
}
int sa3ds_ring_groups_open(SA3DSRings*r,const SA3DSPak*pak){
    SA3DSPakChunk chunk;SA3DSPakResult found=sa3dspak_find(pak,"RING_GROUPS",&chunk);
    if(found==SA3DSPAK_ERR_NOT_FOUND)return 1;
    if(found!=SA3DSPAK_OK||chunk.size<32+32+4100)return 0;
    const uint8_t*b=chunk.data;uint32_t n=r32(b+12),off=r32(b+16),spec=r32(b+20),trig=r32(b+24),total=r32(b+28);
    if(memcmp(b,"SA3DRGRP",8)||r16(b+8)!=1||r16(b+10)||!n||n>2046||off!=32||
       spec!=off+n*40||trig!=spec+32||total!=trig+4100||chunk.size!=total)return 0;
    float load=rf(b+spec),unload=rf(b+spec+4),child=rf(b+spec+8);
    if(!isfinite(load)||load<=0||!isfinite(unload)||unload<load||!isfinite(child)||child<=0||
       r32(b+spec+12)!=7||r32(b+spec+16)!=1025||r32(b+spec+20)||r32(b+spec+24)||r32(b+spec+28))return 0;
    float previous=-1;
    for(unsigned i=0;i<1025;i++){
        float v=rf(b+trig+i*4);
        if(!isfinite(v)||v<0||v>1||v<previous)return 0;
        previous=v;
    }
    if(rf(b+trig)!=0||rf(b+trig+4096)!=1)return 0;
    SA3DSRingPlacement prev={0},p;uint32_t actors=r->count;
    for(uint32_t i=0;i<n;i++){
        placement(b+off+i*40,&p);
        if(p.act>1||p.source_row>=1023||(p.type&0xfff)!=21||
           !isfinite(p.position.x)||!isfinite(p.position.y)||!isfinite(p.position.z))return 0;
        for(int j=0;j<3;j++)if(!isfinite(p.parameters[j]))return 0;
        if(p.parameters[0]<0||p.parameters[0]>=8||p.parameters[2]<0||p.parameters[2]>=2)return 0;
        if(i&&(p.act<prev.act||(p.act==prev.act&&p.source_row<=prev.source_row)))return 0;
        /* One original SET slot cannot be simultaneously a direct ring/group. */
        for(uint32_t j=0;j<r->count;j++){
            SA3DSRingPlacement direct;sa3ds_rings_placement(r,j,&direct);
            if(direct.act==p.act&&direct.source_row==p.source_row)return 0;
        }
        actors+=(uint32_t)p.parameters[0]+((int)p.parameters[2]==0?1u:0u);
        if(actors>SA3DS_RING_MAX_ROWS)return 0;
        prev=p;
    }
    r->groups=calloc(n,sizeof(*r->groups));r->actors=calloc(actors,sizeof(*r->actors));
    if(!r->groups||!r->actors)return 0;
    /* Read direct rows before publishing the derived actor cache. */
    SA3DSRingPlacement *cache=r->actors;r->actors=NULL;
    for(uint32_t i=0;i<r->count;i++)sa3ds_rings_placement(r,i,&cache[i]);
    r->actors=cache;r->quarter_lut=b+trig;r->group_count=n;
    r->group_load_squared=load;r->group_unload_squared=unload;r->child_unload_squared=child;
    uint32_t cursor=r->count;
    for(uint32_t i=0;i<n;i++){
        SA3DSRingGroup*g=&r->groups[i];placement(b+off+i*40,&g->placement);g->first_child=cursor;
        unsigned first=(int)g->placement.parameters[2]==0?0:1,last=(unsigned)g->placement.parameters[0];
        for(unsigned j=first;j<=last;j++){
            SA3DSRingPlacement *a=&cache[cursor++];
            a->act=g->placement.act;a->source_row=g->placement.source_row;a->group=(int32_t)i;a->child_index=(uint8_t)j;
            a->position=child_position(r,&g->placement,j);
            if(!isfinite(a->position.x)||!isfinite(a->position.y)||!isfinite(a->position.z))return 0;
        }
        g->child_count=cursor-g->first_child;
    }
    r->count=cursor;return cursor==actors;
}
static void clear_children(SA3DSRings*r,SA3DSRingGroup*g){
    for(uint32_t j=0;j<g->child_count;j++){
        uint32_t i=g->first_child+j;
        uint8_t consumed=(uint8_t)((g->journal&(1u<<r->actors[i].child_index))!=0);
        r->states[i]=(SA3DSRingState){.collected=consumed,.consumed=consumed};
    }
}
void sa3ds_ring_groups_reset(SA3DSRings*r,uint32_t act){
    for(uint32_t i=0;i<r->group_count;i++)if(r->groups[i].placement.act==act){
        r->groups[i].action=0;r->groups[i].journal=0;r->groups[i].skip_children=1;
    }
}
void sa3ds_ring_groups_begin(SA3DSRings*r,SA1Vec3 origin,unsigned clip){
    for(uint32_t i=0;i<r->group_count;i++){
        SA3DSRingGroup*g=&r->groups[i];g->skip_children=1;if(g->placement.act!=r->act)continue;
        if(g->action==3){g->action=4;continue;}
        if(g->action==4){g->action=0;clear_children(r,g);continue;}
        float x=origin.x-g->placement.position.x,y=origin.y-g->placement.position.y,z=origin.z-g->placement.position.z;
        float distance=x*x+y*y+z*z;
        if(!g->action){
            unsigned high=(g->placement.type>>12)&7;
            int allowed=!high||(high==1?clip<1:clip<2);
            if(allowed&&distance<=r->group_load_squared){g->action=1;clear_children(r,g);}
            continue; /* first parent update creates, but does not run children */
        }
        if(distance>r->group_unload_squared){g->action=4;continue;}
        g->skip_children=0;
    }
}
void sa3ds_ring_groups_end(SA3DSRings*r){
    for(uint32_t i=0;i<r->group_count;i++){
        SA3DSRingGroup*g=&r->groups[i];if(g->placement.act!=r->act||g->action!=1||g->skip_children)continue;
        for(uint32_t j=0;j<g->child_count;j++){
            uint32_t at=g->first_child+j;SA3DSRingState*s=&r->states[at];
            if(s->action==3)g->journal|=(uint8_t)(1u<<r->actors[at].child_index);
            if(s->distance_unloaded)g->action=3;
        }
    }
}
