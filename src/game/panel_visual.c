#include "panel_visual.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return p[0]|((uint16_t)p[1]<<8);}
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static int32_t ri(const uint8_t*p){uint32_t u=r32(p);int32_t s;memcpy(&s,&u,4);return s;}
int sa3ds_panel_visual_open(SA3DSPanelVisual*out,const void*data,size_t size){
    if(!out)return 0;
    memset(out,0,sizeof(*out));if(!data||size>SA3DS_PANEL_VISUAL_MAX_BYTES)return 0;
    SA3DSPak pak;SA3DSPakChunk config;SA3DSPanelVisual next={0};
    if(sa3dspak_open(&pak,data,size)!=SA3DSPAK_OK||pak.toc_count!=5||
       sa3dspak_find(&pak,"PANEL",&config)!=SA3DSPAK_OK||config.flags||config.size!=944||
       memcmp(config.data,"SA3PANL\0",8)||r32(config.data+8)!=1||r32(config.data+12)!=3||
       r32(config.data+16)!=8||r32(config.data+20)!=32||r32(config.data+24)!=80||r32(config.data+28)!=944)return 0;
    next.nodes=config.data+32;next.poses=config.data+80;
    for(unsigned i=0;i<3;i++){
        const uint8_t *n=next.nodes+i*16;
        if(r16(n)!=(i?0:65535)||r16(n+2)!=i||r32(n+4)&~0x3fu||r32(n+8)||r32(n+12))return 0;
        char key[16];snprintf(key,sizeof(key),"VISUAL%u",i);SA3DSPakChunk c;
        if(sa3dspak_find(&pak,key,&c)!=SA3DSPAK_OK||c.flags||
           sa3ds_ec_visual_open(&next.components[i],c.data,(size_t)c.size)!=SA3DS_EC_VISUAL_OK)return 0;
        SA3DSECVisual *v=&next.components[i];
        if(v->model_count!=1||v->instance_count!=1||v->vertex_count>2048||v->index_count>4096||
           v->material_count>16||v->textures.texture_count>16)return 0;
        size_t bytes=0;
        for(uint32_t t=0;t<v->textures.texture_count;t++){
            SA3TTexture texture;const uint8_t *rgba;
            if(sa3t_texture(&v->textures,t,&texture,&rgba)!=SA3T_OK||texture.width<8||texture.height<8||
               texture.width>256||texture.height>256||(texture.width&(texture.width-1))||(texture.height&(texture.height-1)))return 0;
            bytes+=texture.data_size;
        }
        if(bytes>256u*1024u)return 0;
    }
    for(unsigned j=0;j<24;j++){
        const uint8_t *p=next.poses+j*36;
        for(unsigned a=0;a<3;a++)
            if(!isfinite(rf(p+a*4))||fabsf(rf(p+a*4))>4096||
               ri(p+12+a*4)<-16777216||ri(p+12+a*4)>16777216||
               !isfinite(rf(p+24+a*4))||fabsf(rf(p+24+a*4))>128)return 0;
    }
    *out=next;return 1;
}
int sa3ds_panel_visual_pose(const SA3DSPanelVisual*v,unsigned frame,SA3CMatrix world[3]){
    if(!v||!v->nodes||!v->poses||!world||frame>=8)return 0;
    for(unsigned i=0;i<3;i++){
        const uint8_t *p=v->poses+(frame*3+i)*36;
        float unit=6.28318530717958647692f/65536.f;
        SA3CVec3 position={rf(p),rf(p+4),rf(p+8)};
        SA3CVec3 rotation={ri(p+12)*unit,ri(p+16)*unit,ri(p+20)*unit};
        SA3CVec3 scale={rf(p+24),rf(p+28),rf(p+32)};
        SA3CMatrix local;
        /* Animated evaluation applies all TRS despite static UNIT/BREAK bits. */
        sa3c_matrix_build_object(&local,position,rotation,scale,r32(v->nodes+i*16+4)&~7u);
        if(i){
            for(unsigned c=0;c<4;c++)for(unsigned row=0;row<4;row++)
                world[i].m[c*4+row]=world[0].m[row]*local.m[c*4]+
                    world[0].m[4+row]*local.m[c*4+1]+world[0].m[8+row]*local.m[c*4+2]+
                    world[0].m[12+row]*local.m[c*4+3];
        }else world[0]=local;
    }
    return 1;
}
