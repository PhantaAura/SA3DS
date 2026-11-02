#include "saru.h"
#include "../sa3dspak/sa3dspak.h"
#include <math.h>
#include <string.h>
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
int sa3ds_saru_definitions_open(SA3DSSaruDefinitions*out,const void*data,size_t size){
    if(!out)return 0;
    memset(out,0,sizeof(*out));if(!data||size>SA3DS_SARU_PACKAGE_MAX_BYTES)return 0;
    SA3DSPak pak;SA3DSPakChunk c,metadata;
    if(sa3dspak_open(&pak,data,size)!=SA3DSPAK_OK||pak.toc_count!=2||
       sa3dspak_find(&pak,"SARU",&c)!=SA3DSPAK_OK||sa3dspak_find(&pak,"METADATA",&metadata)!=SA3DSPAK_OK||
       c.flags||metadata.flags||c.size<112||memcmp(c.data,"SA3SARU\0",8)||r32(c.data+8)!=1)return 0;
    uint32_t count=r32(c.data+12),co=r32(c.data+24);
    if(count>2046||r32(c.data+16)!=64||r32(c.data+20)!=40||co!=64+count*40||
       r32(c.data+28)!=48||r32(c.data+32)!=co+48||c.size!=co+48||r32(c.data+36)!=3||
       r32(c.data+40)!=2||rf(c.data+44)!=360000.f||rf(c.data+48)!=490000.f||
       !isfinite(rf(c.data+52))||rf(c.data+52)<=0||rf(c.data+52)>1024||
       !isfinite(rf(c.data+56))||rf(c.data+56)<=0||rf(c.data+56)>128||r32(c.data+60))return 0;
    SA3DSSaruDefinitions next={.placements={c.data+64,0,count},.collider=c.data+co,
        .load_squared=rf(c.data+44),.unload_squared=rf(c.data+48),.visibility_radius=rf(c.data+52),.display_scale=rf(c.data+56)};
    SA3DSMovementPlacement previous={0},p;
    for(uint32_t i=0;i<count;i++){
        if(!sa3ds_movement_objects_placement(&next.placements,i,&p)||p.act>1||p.source_row>=1023||
           (p.type&4095u)!=29||!finite3(p.position))return 0;
        for(unsigned a=0;a<3;a++)if(!isfinite(p.parameters[a]))return 0;
        if(i&&(p.act<previous.act||(p.act==previous.act&&p.source_row<=previous.source_row)))return 0;
        previous=p;
    }
    if(next.collider[0]||next.collider[1])return 0; /*profile uses one form0 sphere*/
    for(unsigned i=0;i<7;i++)if(!isfinite(rf(next.collider+8+i*4))||fabsf(rf(next.collider+8+i*4))>1024)return 0;
    if(rf(next.collider+20)<=0)return 0;
    *out=next;return 1;
}
int sa3ds_saru_initialize(const SA3DSMovementPlacement*p,uint16_t random15,double sx,double cx,SA3DSSaruInitial*out){
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    if(!p||(p->type&4095u)!=29||random15>32767||!finite3(p->position)||!isfinite(sx)||!isfinite(cx)||fabs(sx)>1||fabs(cx)>1)return 0;
    for(unsigned i=0;i<3;i++)if(!isfinite(p->parameters[i]))return 0;
    float radius=p->parameters[2]>1.f?p->parameters[2]:1.f;
    /*Ninja's Y rotation is stored in a float matrix before CalcPoint. */
    float dx=(float)((double)(float)cx*radius),dz=(float)(-(double)(float)sx*radius);
    SA3DSSaruInitial next={.spawn_origin=p->position,
        .task_position={(float)((double)p->position.x+dx),p->position.y,(float)((double)p->position.z+dz)},
        .task_scale={1,1,radius},.task_angles={p->angles[0],p->angles[1],(uint16_t)p->angles[2]>>8},
        .initial_angle_y=p->angles[1],.packed_tag_low=(uint16_t)p->angles[2]&255u,
        .task_status=(uint16_t)(0x100u|(p->parameters[0]<0?0x2000u:0u)),.enemy_flags=0x483,
        .mode=p->parameters[1]<0?1:p->parameters[1]>=1000?2:0,
        .index_seed=(uint8_t)((uint32_t)random15*20u/32768u),.animation_frame=3,
        .behavior_flags=0xe0,.parameter_cc=1.6f,.parameter_d0=1,.parameter_3c=.1f};
    if(!finite3(next.task_position))return 0;
    *out=next;return 1;
}
