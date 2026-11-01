#include "emerald_coast_route.h"
#include "sa3dspak.h"
#include <math.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
static uint32_t r32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static SA1Vec3 vec(const uint8_t*p){return (SA1Vec3){rf(p),rf(p+4),rf(p+8)};}
static int finite3(SA1Vec3 v){return isfinite(v.x)&&isfinite(v.y)&&isfinite(v.z);}
const SA3DSECStart *sa3ds_ec_route_start(const SA3DSECRoute*r,uint32_t act){
    if(!r || r->start_count>SA3DS_EC_ROUTE_MAX_ACTS)return NULL;
    for(uint32_t i=0;i<r->start_count;i++)if(r->starts[i].act_index==act)return &r->starts[i];
    return NULL;
}
int sa3ds_ec_route_open(SA3DSECRoute*out,const void*data,size_t size){
    if(!out)return 0;
    memset(out,0,sizeof(*out));SA3DSPak pak;SA3DSPakChunk chunk;
    if(!data||sa3dspak_open(&pak,data,size)!=SA3DSPAK_OK||
       sa3dspak_find(&pak,"ROUTE",&chunk)!=SA3DSPAK_OK||chunk.size<32)return 0;
    const uint8_t*b=chunk.data;
    if(memcmp(b,"SA3DECRO",8)||r16(b+8)!=1||r16(b+10)||r32(b+12)!=1)return 0;
    uint32_t ns=r32(b+16),ne=r32(b+20),so=r32(b+24),eo=r32(b+28);
    if(!ns||ns>SA3DS_EC_ROUTE_MAX_ACTS||ne>SA3DS_EC_ROUTE_MAX_ACTS||so!=32||
       eo!=so+ns*24u||chunk.size!=(size_t)eo+ne*24u)return 0;
    SA3DSECRoute route={.start_count=ns,.edge_count=ne};
    for(uint32_t i=0;i<ns;i++){
        const uint8_t*p=b+so+i*24u;uint32_t act=r32(p),yaw=r32(p+20);
        SA1Vec3 origin=vec(p+8);
        if(act>=SA3DS_EC_ROUTE_MAX_ACTS||r32(p+4)!=0||yaw>65535||!finite3(origin))return 0;
        for(uint32_t j=0;j<i;j++)if(route.starts[j].act_index==act)return 0;
        route.starts[i]=(SA3DSECStart){act,origin,(uint16_t)yaw};
    }
    for(uint32_t i=0;i<ne;i++){
        const uint8_t*p=b+eo+i*24u;uint32_t from=r32(p),to=r32(p+4);
        SA1Vec3 center=vec(p+8);float radius=rf(p+20);
        if(from==to||!finite3(center)||!isfinite(radius)||radius<=0||
           !sa3ds_ec_route_start(&route,from)||!sa3ds_ec_route_start(&route,to))return 0;
        for(uint32_t j=0;j<i;j++)if(route.edges[j].from_act==from)return 0;
        route.edges[i]=(SA3DSECActEdge){from,to,center,radius};
    }
    *out=route;return 1;
}
int sa3ds_ec_route_next(const SA3DSECRoute*r,uint32_t act,SA1Vec3 p,uint32_t*next){
    if(!r||!next||!finite3(p)||r->edge_count>SA3DS_EC_ROUTE_MAX_ACTS)return 0;
    for(uint32_t i=0;i<r->edge_count;i++){
        const SA3DSECActEdge*edge=&r->edges[i];
        if(edge->from_act!=act)continue;
        float dx=p.x-edge->center.x,dy=p.y-edge->center.y,dz=p.z-edge->center.z;
        if(sqrtf(dx*dx+dy*dy+dz*dz)<edge->radius){*next=edge->to_act;return 1;}
    }
    return 0;
}
float sa3ds_ec_route_player_yaw(uint16_t angle){
    const float pi=3.14159265358979323846f;
    float yaw=pi*.5f-(float)angle*(2.0f*pi/65536.0f);
    if(yaw<-pi)yaw+=2.0f*pi;
    return yaw;
}
