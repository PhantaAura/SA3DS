#include "movement_objects.h"
#include "../sa3dspak/sa3dspak.h"
#include <math.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
static uint32_t r32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
int sa3ds_movement_objects_placement(const SA3DSMovementObjects*o,uint32_t i,SA3DSMovementPlacement*out){
    if(!o||!out||i>=o->count||!o->rows)return 0;
    const uint8_t*p=o->rows+i*40;
    *out=(SA3DSMovementPlacement){.act=r32(p),.source_row=r32(p+4),.type=r16(p+8)};
    for(int j=0;j<3;j++){uint16_t a=r16(p+10+j*2);memcpy(&out->angles[j],&a,2);out->parameters[j]=rf(p+28+j*4);}
    out->position=(SA1Vec3){rf(p+16),rf(p+20),rf(p+24)};
    return 1;
}
int sa3ds_movement_objects_open(SA3DSMovementObjects*out,const void*data,size_t size){
    if(!out)return 0;
    *out=(SA3DSMovementObjects){0};
    SA3DSPak pak;SA3DSPakChunk c;unsigned found=0;
    if(sa3dspak_open(&pak,data,size)!=SA3DSPAK_OK)return 0;
    for(uint32_t i=0;i<pak.toc_count;i++){
        SA3DSPakChunk item;
        if(sa3dspak_chunk_at(&pak,i,&item)!=SA3DSPAK_OK)return 0;
        if(!strcmp(item.type,"MOVE_OBJECTS")){c=item;found++;}
    }
    if(found!=1||c.flags||c.size<80)return 0;
    const uint8_t*b=c.data;
    uint32_t n=r32(b+12),off=r32(b+16),spec=r32(b+20),total=r32(b+28);
    uint16_t revision=r16(b+10);
    static const uint16_t kinds[]={1,2,3,18,26,40};
    static const uint8_t lists[]={2,2,3,2,2,3}, flags[]={2,2,3,2,2,6};
    unsigned count=revision?6:3;
    if(memcmp(b,"SA3DMOVE",8)||r16(b+8)!=1||revision>1||n>2046||off!=32||
       spec!=32+n*40||r32(b+24)!=count||total!=spec+count*16||total!=c.size)return 0;
    for(unsigned i=0;i<count;i++){
        const uint8_t*p=b+spec+i*16;
        float load=i==3?4000000.f:360000.f;
        if(r16(p)!=kinds[i]||p[2]!=lists[i]||p[3]!=flags[i]||
           rf(p+4)!=load||rf(p+8)!=load+100.f||r32(p+12))return 0;
    }
    SA3DSMovementObjects candidate={b+off,b+spec,n};
    SA3DSMovementPlacement prev={0},p;
    for(uint32_t i=0;i<n;i++){
        if(!sa3ds_movement_objects_placement(&candidate,i,&p))return 0;
        unsigned kind=p.type&4095;
        int known=(kind>=1&&kind<=3)||(revision&&(kind==18||kind==26||kind==40));
        if(p.act>1||p.source_row>=1023||!known||!isfinite(p.position.x)||!isfinite(p.position.y)||!isfinite(p.position.z))return 0;
        for(int j=0;j<3;j++)if(!isfinite(p.parameters[j]))return 0;
        if(i&&(p.act<prev.act||(p.act==prev.act&&p.source_row<=prev.source_row)))return 0;
        prev=p;
    }
    *out=candidate;return 1;
}
int sa3ds_movement_spring_command(const SA3DSMovementPlacement*p,SA3DSSpringCommand*out){
    if(!out)return 0;
    *out=(SA3DSSpringCommand){0};
    if(!p||((p->type&4095)!=1&&(p->type&4095)!=2)||!isfinite(p->parameters[1]))return 0;
    float strength=p->parameters[1]+5.f;
    if(!isfinite(strength))return 0;
    int16_t timer=0;
    if((p->type&4095)==2){
        double value=p->parameters[0];
        if(!isfinite(value)||value>=2147483648.0)return 0;
        if(value<0)value=0;
        uint16_t low=(uint16_t)(uint32_t)value;
        memcpy(&timer,&low,2);
    }
    *out=(SA3DSSpringCommand){strength,timer,(uint8_t)((p->type&4095)==1?3:4),
        (uint8_t)((p->type&4095)==1?9:10),16};
    return 1;
}

int sa3ds_numbered_panels_open(SA3DSNumberedPanels *out,const SA3DSMovementObjects *objects,unsigned act){
    if(!out)return 0;
    *out=(SA3DSNumberedPanels){0};
    if(!objects||act>1)return 0;
    SA3DSNumberedPanels panels={0};
    for(uint32_t index=0;index<objects->count;index++){
        SA3DSMovementPlacement p;
        if(!sa3ds_movement_objects_placement(objects,index,&p))return 0;
        if(p.act!=act||(p.type&4095u)!=18)continue;
        /* 4B8DC0 checks the range, then truncates abs(Scale.x). */
        unsigned slot=p.parameters[0]>0.f&&p.parameters[0]<10.f?(unsigned)p.parameters[0]:0;
        unsigned bit=1u<<slot;
        if(panels.present&bit)return 0;
        panels.present|=bit;
        panels.slots[slot]=p;
    }
    *out=panels;
    return 1;
}
int sa3ds_numbered_panel_next(const SA3DSNumberedPanels *panels,unsigned from){
    if(!panels||from==0||from>=SA3DS_NUMBERED_PANEL_SLOTS||!(panels->present&(1u<<from)))return -1;
    unsigned next=from+1;
    if(next<SA3DS_NUMBERED_PANEL_SLOTS&&(panels->present&(1u<<next)))return (int)next;
    return panels->present&1u?0:-1;
}
int sa3ds_numbered_panel_step(SA1Vec3 task,SA1Vec3 velocity,SA1Vec3 aim,SA1Vec3 landing,
    SA1Vec3 *next_task,SA1Vec3 *next_velocity){
    if(!next_task||!next_velocity||!isfinite(landing.x)||!isfinite(landing.y)||!isfinite(landing.z))return 0;
    SA1Vec3 delta={aim.x-task.x,aim.y-task.y,aim.z-task.z};
    float distance=sqrtf(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
    float speed=sqrtf(velocity.x*velocity.x+velocity.y*velocity.y+velocity.z*velocity.z);
    if(!isfinite(distance)||!isfinite(speed))return 0;
    if(distance<speed){
        *next_task=landing;
        *next_velocity=(SA1Vec3){0};
        return 2;
    }
    if(speed<9.f)speed=9.f;
    else if(speed<15.f)speed+=0.5f;
    float scale=distance>0.f?speed/distance:0.f;
    *next_velocity=(SA1Vec3){delta.x*scale,delta.y*scale,delta.z*scale};
    *next_task=(SA1Vec3){task.x+next_velocity->x,task.y+next_velocity->y,task.z+next_velocity->z};
    return 1;
}
