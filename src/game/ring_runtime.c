#include "ring_runtime.h"
#include "sa3dspak.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
static int16_t rs16(const uint8_t*p){uint16_t u=r16(p);int16_t s;memcpy(&s,&u,2);return s;}
static uint32_t r32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static SA1Vec3 rv(const uint8_t*p){return (SA1Vec3){rf(p),rf(p+4),rf(p+8)};}
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static float distance2(SA1Vec3 a,SA1Vec3 b){float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;}
int sa3ds_rings_placement(const SA3DSRings*r,uint32_t i,SA3DSRingPlacement*out){
    if(!r||!out||i>=r->count||!r->rows)return 0;
    const uint8_t*p=r->rows+i*40;out->act=r32(p);out->source_row=r32(p+4);p+=8;
    out->type=r16(p);for(int j=0;j<3;j++)out->angles[j]=rs16(p+2+j*2);
    out->position=rv(p+8);for(int j=0;j<3;j++)out->parameters[j]=rf(p+20+j*4);
    return 1;
}
void sa3ds_rings_destroy(SA3DSRings*r){if(r){free(r->states);memset(r,0,sizeof(*r));}}
int sa3ds_rings_open(SA3DSRings*out,const void*data,size_t size){
    if(!out)return 0;
    memset(out,0,sizeof(*out));SA3DSPak pak;SA3DSPakChunk chunk;
    if(!data||sa3dspak_open(&pak,data,size)!=SA3DSPAK_OK||
       sa3dspak_find(&pak,"RINGS",&chunk)!=SA3DSPAK_OK||chunk.size<64)return 0;
    const uint8_t*b=chunk.data;
    uint32_t count=r32(b+16),row=r32(b+20),spec=r32(b+24),total=r32(b+28);
    if(memcmp(b,"SA3DRING",8)||r16(b+8)!=1||r16(b+10)||r32(b+12)!=2||
       !count||count>SA3DS_RING_MAX_ROWS||row!=32||spec!=row+count*40||
       total!=spec+32||chunk.size!=total)return 0;
    SA3DSRings next={.rows=b+row,.count=count,.collision_center=rv(b+spec),
        .radius=rf(b+spec+12),.load_squared=rf(b+spec+16),.unload_squared=rf(b+spec+20)};
    uint32_t turn=r32(b+spec+24);
    if(!finite3(next.collision_center)||!isfinite(next.radius)||next.radius<=0||
       !isfinite(next.load_squared)||next.load_squared<=0||!isfinite(next.unload_squared)||
       next.unload_squared<next.load_squared||turn>65535||r32(b+spec+28)>2)return 0;
    next.turn=(uint16_t)turn;next.clip_level=r32(b+spec+28);
    SA3DSRingPlacement previous={0},p;
    for(uint32_t i=0;i<count;i++){
        if(!sa3ds_rings_placement(&next,i,&p)||p.act>1||p.source_row>=1023||
           (p.type&0xfff)||!finite3(p.position))return 0;
        for(int j=0;j<3;j++)if(!isfinite(p.parameters[j]))return 0;
        if(i&&(p.act<previous.act||(p.act==previous.act&&p.source_row<=previous.source_row)))return 0;
        previous=p;
    }
    next.states=calloc(count,sizeof(*next.states));if(!next.states)return 0;
    *out=next;sa3ds_rings_reset_act(out,0);return 1;
}
void sa3ds_rings_reset_act(SA3DSRings*r,uint32_t act){
    if(!r||!r->states||act>1)return;
    r->act=act;
    for(uint32_t i=0;i<r->count;i++){
        SA3DSRingPlacement p;sa3ds_rings_placement(r,i,&p);
        if(p.act==act)r->states[i]=(SA3DSRingState){.yaw=(uint16_t)p.angles[1]};
    }
}
unsigned sa3ds_rings_add(SA3DSRings*r,int16_t amount){
    if(!r)return 0;
    int old=r->rings;
    uint16_t sum=(uint16_t)((uint16_t)r->rings+(uint16_t)amount);
    memcpy(&r->rings,&sum,2);
    int awards=(int)r->rings/100-old/100;
    if(awards>0){r->extra_lives+=(unsigned)awards;return (unsigned)awards;}
    return 0;
}
uint32_t sa3ds_rings_tick(SA3DSRings*r,SA1Vec3 origin,SA1Vec3 sphere,float radius,
    int debug,int eligible,unsigned clip){
    if(!r||!r->states||!finite3(origin)||!finite3(sphere)||!isfinite(radius)||radius<=0||clip>2)return 0;
    uint32_t collected=0;
    for(uint32_t i=0;i<r->count;i++){
        SA3DSRingPlacement p;sa3ds_rings_placement(r,i,&p);
        SA3DSRingState*s=&r->states[i];if(p.act!=r->act||s->collected)continue;
        float d2=distance2(origin,p.position);
        if(!s->action){
            unsigned high=(p.type>>12)&7;
            int allowed=!high||(high==1?clip<1:clip<2);
            if(!allowed||d2>r->load_squared)continue;
            s->action=4; /* source0->4, then4->5, then5->1 on consecutive ticks */
            continue;
        }
        if(d2>r->unload_squared){s->action=0;s->pending_hit=0;s->yaw=(uint16_t)p.angles[1];continue;}
        if(s->action==4){s->action=5;continue;}
        if(s->action==5){s->action=1;continue;}
        if(debug)continue;
        /* The object consumes the previous post-object collision result.
         * A newly registered sphere cannot collect on its first active tick. */
        if(s->pending_hit){
            s->collected=1;s->pending_hit=0;
            if(eligible){sa3ds_rings_add(r,1);r->collected_events++;collected++;}
            continue;
        }
        SA1Vec3 center={p.position.x+r->collision_center.x,
            p.position.y+r->collision_center.y,p.position.z+r->collision_center.z};
        float sum=radius+r->radius;
        s->pending_hit=distance2(sphere,center)<sum*sum;
        s->yaw=(uint16_t)(s->yaw+r->turn);
    }
    return collected;
}
