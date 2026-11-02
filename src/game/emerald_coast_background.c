#include "emerald_coast_background.h"
#include <math.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return(uint16_t)p[0]|((uint16_t)p[1]<<8);}
static uint32_t r32(const uint8_t*p){return(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t n=r32(p);float f;memcpy(&f,&n,4);return f;}
int sa3ds_ec_background_select(const SA3DSECBackground*b,uint32_t act,uint32_t clip,
    SA3DSECBackgroundConfig*out){
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    if(!b||!b->rows||act>1||clip>2)return 0;
    const uint8_t*p=b->rows+(act*3u+clip)*64u;
    out->act=r32(p);out->clip_level=r32(p+4);
    for(unsigned i=0;i<3;i++)out->scale[i]=rf(p+8+i*4);
    out->background_near=rf(p+20);out->background_far=rf(p+24);
    out->terrain_near=rf(p+28);out->terrain_far=rf(p+32);
    out->fog_start=rf(p+36);out->fog_end=rf(p+40);
    out->fog_argb=r32(p+44);out->fog_enabled=r32(p+48);
    out->clear_rgb=r32(p+52);out->flags=r32(p+56);
    return 1;
}
int sa3ds_ec_background_open(SA3DSECBackground*out,const void*data,size_t size){
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    if(!data||size>SA3DS_EC_BACKGROUND_MAX_BYTES)return 0;
    SA3DSECBackground candidate;memset(&candidate,0,sizeof(candidate));
    SA3DSPakChunk cfg,visual,metadata;
    if(sa3dspak_open(&candidate.package,data,size)!=SA3DSPAK_OK||
       candidate.package.toc_count!=3||
       sa3dspak_find(&candidate.package,"METADATA",&metadata)!=SA3DSPAK_OK||
       sa3dspak_find(&candidate.package,"BACKGROUND",&cfg)!=SA3DSPAK_OK||
       sa3dspak_find(&candidate.package,"VISUAL",&visual)!=SA3DSPAK_OK||
       cfg.size!=32u+SA3DS_EC_BACKGROUND_ROWS*64u||
       memcmp(cfg.data,"SA3DBG\0\0",8)||r16(cfg.data+8)!=1||r16(cfg.data+10)!=0||
       r32(cfg.data+12)!=SA3DS_EC_BACKGROUND_ROWS||r32(cfg.data+16)!=64||
       r32(cfg.data+20)!=32||r32(cfg.data+24)!=3||r32(cfg.data+28)!=0||
       sa3ds_ec_visual_open(&candidate.visual,visual.data,(size_t)visual.size)!=SA3DS_EC_VISUAL_OK)
        return 0;
    /* Nested geometry is renderer-only; tiny original background budgets. */
    if(candidate.visual.model_count>8||candidate.visual.instance_count>8||
       candidate.visual.vertex_count>4096||candidate.visual.index_count>16384||
       candidate.visual.material_count>16||candidate.visual.textures.texture_count>16)return 0;
    size_t texture_bytes=0;
    for(uint32_t i=0;i<candidate.visual.textures.texture_count;i++){
        SA3TTexture t;const uint8_t*rgba;
        if(sa3t_texture(&candidate.visual.textures,i,&t,&rgba)!=SA3T_OK||
           t.width<8||t.height<8||t.width>256||t.height>256||
           (t.width&(t.width-1u))||(t.height&(t.height-1u)))return 0;
        texture_bytes+=t.data_size;
    }
    if(texture_bytes>512u*1024u)return 0;
    candidate.rows=cfg.data+32;
    for(uint32_t i=0;i<SA3DS_EC_BACKGROUND_ROWS;i++){
        SA3DSECBackgroundConfig row;
        sa3ds_ec_background_select(&candidate,i/3,i%3,&row);
        if(row.act!=i/3||row.clip_level!=i%3||row.flags!=3||row.fog_enabled>1||
           (row.clear_rgb&0xff000000u)||r32(candidate.rows+i*64u+60)!=0)return 0;
        for(unsigned k=0;k<3;k++)if(!isfinite(row.scale[k])||row.scale[k]<=0)return 0;
        if(!isfinite(row.background_near)||!isfinite(row.background_far)||
           !isfinite(row.terrain_near)||!isfinite(row.terrain_far)||
           !isfinite(row.fog_start)||!isfinite(row.fog_end)||
           row.background_near<=0||row.background_far<=row.background_near||
           row.terrain_near<=0||row.terrain_far<=row.terrain_near||
           row.fog_start<0||row.fog_end<=row.fog_start)return 0;
    }
    *out=candidate;return 1;
}
int sa3ds_ec_background_transform(const SA3DSECBackground*b,uint32_t act,uint32_t clip,
    const float camera[3],SA3CMatrix*out){
    if(!out)return 0;
    memset(out,0,sizeof(*out));out->m[0]=out->m[5]=out->m[10]=out->m[15]=1;
    SA3DSECBackgroundConfig row;
    if(!camera||!isfinite(camera[0])||!isfinite(camera[1])||!isfinite(camera[2])||
       !sa3ds_ec_background_select(b,act,clip,&row))return 0;
    out->m[0]=row.scale[0];out->m[5]=row.scale[1];out->m[10]=row.scale[2];
    out->m[12]=camera[0];out->m[14]=camera[2];return 1;
}
