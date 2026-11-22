#include "automatic_loop.h"
#include "sa3dspak.h"
#include <math.h>
#include <string.h>
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float f32(const uint8_t *p){uint32_t u=u32(p);float f;memcpy(&f,&u,4);return f;}
int sa3ds_automatic_loops_open(SA3DSAutomaticLoops *out,const void *data,size_t size){
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    SA3DSPak pak;SA3DSPakChunk chunk;
    if(sa3dspak_open(&pak,data,size)!=SA3DSPAK_OK||sa3dspak_find(&pak,"LOOP_PATHS",&chunk)!=SA3DSPAK_OK||
        chunk.flags||chunk.size<16||memcmp(chunk.data,"SA3DLOOP",8)||u32(chunk.data+8)!=1)return 0;
    uint32_t count=u32(chunk.data+12);
    if(!count||count>SA3DS_LOOP_MAX_PATHS)return 0;
    size_t offset=16;
    for(uint32_t i=0;i<count;i++){
        if(offset>chunk.size||chunk.size-offset<20)goto fail;
        const uint8_t *p=chunk.data+offset;
        SA3DSAutomaticLoopActor *a=&out->actors[i];
        uint32_t wrap=u32(p+8),n=u32(p+12);
        a->act=u32(p);a->ordinal=u32(p+4);a->path.length=f32(p+16);
        if(a->act>1||wrap>1||n<2||n>512||n>SA3DS_LOOP_MAX_POINTS-out->point_count||
           !isfinite(a->path.length)||a->path.length<=0||chunk.size-offset-20<(size_t)n*16)goto fail;
        for(uint32_t k=0;k<i;k++)if(a->act==out->actors[k].act&&a->ordinal==out->actors[k].ordinal)goto fail;
        a->path.count=n;a->path.wrap=(uint8_t)wrap;
        a->path.points=out->points+out->point_count;
        offset+=20;
        for(uint32_t j=0;j<n;j++,offset+=16){
            p=chunk.data+offset;
            SA1PC2004LoopPoint *point=&out->points[out->point_count++];
            point->length=f32(p);point->position=(SA1Vec3){f32(p+4),f32(p+8),f32(p+12)};
            SA1Vec3 v=point->position;
            if(!isfinite(point->length)||point->length<0||!isfinite(v.x)||!isfinite(v.y)||!isfinite(v.z))goto fail;
            if(j==0)a->minimum=a->maximum=v;
            else{
                a->minimum=(SA1Vec3){fminf(a->minimum.x,v.x),fminf(a->minimum.y,v.y),fminf(a->minimum.z,v.z)};
                a->maximum=(SA1Vec3){fmaxf(a->maximum.x,v.x),fmaxf(a->maximum.y,v.y),fmaxf(a->maximum.z,v.z)};
            }
        }
        a->minimum=sa1_vec3_sub(a->minimum,(SA1Vec3){40,40,40});
        a->maximum=sa1_vec3_add(a->maximum,(SA1Vec3){40,40,40});
        a->cooldown=60;
    }
    if(offset!=chunk.size)goto fail;
    out->count=count;return 1;
fail:
    memset(out,0,sizeof(*out));return 0;
}
void sa3ds_automatic_loops_reset(SA3DSAutomaticLoops *loops,SA1PlayerState *p){
    for(uint32_t i=0;i<loops->count;i++){loops->actors[i].busy=0;loops->actors[i].cooldown=60;}
    p->pc2004_loop=0;p->pc2004_loop_pending=false;p->pc2004_loop_distance=0;
    p->pc2004_status_flags&=(uint16_t)~0x2000u;
}
int sa3ds_automatic_loops_tick(SA3DSAutomaticLoops *loops,uint32_t act,SA1Vec3 v,SA1PlayerState *p){
    int command=0;
    /* Constructors prepend automatic path tasks to source list2. */
    for(uint32_t i=loops->count;i>0;i--){
        SA3DSAutomaticLoopActor *a=&loops->actors[i-1];
        if(a->act!=act)continue;
        if(a->busy){
            if(!(p->pc2004_status_flags&0x2000u)){a->cooldown=0;a->busy=0;}
            continue;
        }
        if(a->cooldown<60)a->cooldown++;
        if(a->cooldown<60||v.x<a->minimum.x||v.y<a->minimum.y||v.z<a->minimum.z||
            v.x>a->maximum.x||v.y>a->maximum.y||v.z>a->maximum.z)continue;
        for(uint32_t j=0;j<a->path.count-1;j++){
            if(sa1_vec3_length(sa1_vec3_sub(v,a->path.points[j].position))>=40)continue;
            p->pc2004_loop=&a->path;p->pc2004_loop_distance=sa1_pc2004_loop_nearest(&a->path,v);
            p->pc2004_loop_pending=true;p->pc2004_status_flags|=0x1000u;
            a->busy=1;command=1;break;
        }
    }
    return command;
}
