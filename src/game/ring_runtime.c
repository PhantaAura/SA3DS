#include "ring_runtime.h"
#include "ring_groups.h"
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
    if(r->actors){*out=r->actors[i];return 1;}
    memset(out,0,sizeof(*out));out->group=-1;
    const uint8_t*p=r->rows+i*40;out->act=r32(p);out->source_row=r32(p+4);p+=8;
    out->type=r16(p);for(int j=0;j<3;j++)out->angles[j]=rs16(p+2+j*2);
    out->position=rv(p+8);for(int j=0;j<3;j++)out->parameters[j]=rf(p+20+j*4);
    return 1;
}
void sa3ds_rings_destroy(SA3DSRings*r){if(r){free(r->states);free(r->actors);free(r->groups);memset(r,0,sizeof(*r));}}
int sa3ds_rings_open(SA3DSRings*out,const void*data,size_t size){
    if(!out)return 0;
    memset(out,0,sizeof(*out));SA3DSPak pak;SA3DSPakChunk chunk;
    if(!data||sa3dspak_open(&pak,data,size)!=SA3DSPAK_OK||
       sa3dspak_find(&pak,"RINGS",&chunk)!=SA3DSPAK_OK||chunk.size<64)return 0;
    if(pak.toc_count>3)return 0;
    unsigned seen=0;
    for(uint32_t i=0;i<pak.toc_count;i++){
        SA3DSPakChunk c;if(sa3dspak_chunk_at(&pak,i,&c)!=SA3DSPAK_OK)return 0;
        unsigned bit=!strcmp(c.type,"RINGS")?1:!strcmp(c.type,"METADATA")?2:!strcmp(c.type,"RING_GROUPS")?4:0;
        if(!bit||(seen&bit))return 0;
        seen|=bit;
    }
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
    next.direct_count=count;
    if(!sa3ds_ring_groups_open(&next,&pak)){sa3ds_rings_destroy(&next);return 0;}
    next.states=calloc(next.count,sizeof(*next.states));
    if(!next.states){sa3ds_rings_destroy(&next);return 0;}
    *out=next;sa3ds_rings_reset_act(out,0);return 1;
}
void sa3ds_rings_reset_act(SA3DSRings*r,uint32_t act){
    if(!r||!r->states||act>1)return;
    r->act=act;sa3ds_ring_groups_reset(r,act);
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
static unsigned actor_tick(SA3DSRings*r,uint32_t i,SA1Vec3 origin,SA1Vec3 sphere,
    float radius,int debug,int eligible,unsigned clip,int presented,unsigned*queued){
    SA3DSRingPlacement p;sa3ds_rings_placement(r,i,&p);
    SA3DSRingState*s=&r->states[i];if(p.act!=r->act||s->consumed)return 0;
    if(p.group>=0){
        SA3DSRingGroup*g=&r->groups[p.group];
        if(g->action!=1||g->skip_children)return 0;
    }
    float d2=distance2(origin,p.position);
    if(!s->action){
        if(p.group<0){
            unsigned high=(p.type>>12)&7;
            int allowed=!high||(high==1?clip<1:clip<2);
            if(!allowed||d2>r->load_squared)return 0;
        }
        if(p.group>=0&&d2>r->child_unload_squared){s->distance_unloaded=1;return 0;}
        s->action=4;return 0;
    }
    float unload=p.group<0?r->unload_squared:r->child_unload_squared;
    if(d2>unload){
        *s=(SA3DSRingState){.yaw=(uint16_t)p.angles[1],.distance_unloaded=(uint8_t)(p.group>=0)};
        return 0;
    }
    if(s->action==4){s->action=5;return 0;}
    if(s->action==5){s->action=1;return 0;}
    if(s->action==3){s->consumed=1;s->action=0;return 0;}
    if(s->action==2){
        /* Original third particle age starts-4, advances source0.8f each
         * unpaused update, and changes action only on a draw-enabled update. */
        s->pickup_normal_draw=0;s->pickup_age+=0.8f;
        if(presented&&(int)s->pickup_age>7)s->action=3;
        return 0;
    }
    if(debug)return 0;
    unsigned result=0;
    if(s->pending_hit){
        s->collected=1;s->action=2;s->pickup_age=-4;s->pickup_normal_draw=1;
        if(eligible){sa3ds_rings_add(r,1);r->collected_events++;if(p.group>=0)r->group_collected_events++;result=1;}
    }
    /* Original list7 has128 slots. A pickup update still submits its sphere;
     * its ordinary ring display also runs once before particle-only updates. */
    SA1Vec3 center={p.position.x+r->collision_center.x,
        p.position.y+r->collision_center.y,p.position.z+r->collision_center.z};
    float sum=radius+r->radius;
    s->pending_hit=(*queued<128)&&distance2(sphere,center)<sum*sum;
    if(*queued<128)(*queued)++;
    s->yaw=(uint16_t)(s->yaw+r->turn);
    return result;
}
uint32_t sa3ds_rings_tick_presented(SA3DSRings*r,SA1Vec3 origin,SA1Vec3 sphere,float radius,
    int debug,int eligible,unsigned clip,int presented){
    if(!r||!r->states||!finite3(origin)||!finite3(sphere)||!isfinite(radius)||radius<=0||clip>2)return 0;
    uint32_t collected=0;unsigned queued=0;
    sa3ds_ring_groups_begin(r,origin,clip);
    for(uint32_t i=0;i<r->direct_count;i++)collected+=actor_tick(r,i,origin,sphere,radius,debug,eligible,clip,presented,&queued);
    for(uint32_t i=0;i<r->group_count;i++){
        SA3DSRingGroup*g=&r->groups[i];
        for(uint32_t j=g->child_count;j>0;j--)
            collected+=actor_tick(r,g->first_child+j-1,origin,sphere,radius,debug,eligible,clip,presented,&queued);
    }
    sa3ds_ring_groups_end(r);return collected;
}
uint32_t sa3ds_rings_tick(SA3DSRings*r,SA1Vec3 origin,SA1Vec3 sphere,float radius,
    int debug,int eligible,unsigned clip){
    return sa3ds_rings_tick_presented(r,origin,sphere,radius,debug,eligible,clip,1);
}

int sa3ds_rings_draw_normal(SA3DSRings*r,uint32_t i){
    if(!r||!r->states||i>=r->count)return 0;
    SA3DSRingState*s=&r->states[i];
    if(s->action==1)return 1;
    if(s->action==2&&s->pickup_normal_draw){s->pickup_normal_draw=0;return 1;}
    return 0;
}
