#include "spring_visual.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
int sa3ds_spring_visual_open(SA3DSSpringVisual*out,const void*bytes,size_t size){
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    if(!bytes||size>SA3DS_SPRING_VISUAL_MAX_BYTES)return 0;
    SA3DSPak pak;SA3DSPakChunk c;
    SA3DSSpringVisual next={0};
    if(sa3dspak_open(&pak,bytes,size)!=SA3DSPAK_OK||pak.toc_count!=8||
       sa3dspak_find(&pak,"SPRING",&c)!=SA3DSPAK_OK||c.flags||c.size!=224||
       memcmp(c.data,"SA3SPRV\0",8)||r32(c.data+8)!=1||r32(c.data+12)!=6||
       r32(c.data+16)!=32||r32(c.data+20)!=32||r32(c.data+24)||r32(c.data+28)!=224)return 0;
    for(unsigned i=0;i<6;i++){
        const uint8_t *row=c.data+32+i*32;
        if(r32(row)!=i/3+1||r32(row+4)!=i%3||r32(row+24)||r32(row+28))return 0;
        for(unsigned j=0;j<3;j++){
            next.offsets[i][j]=rf(row+8+j*4);
            if(!isfinite(next.offsets[i][j])||fabsf(next.offsets[i][j])>1024)return 0;
        }
        next.cap_displacement[i]=rf(row+20);
        if(!isfinite(next.cap_displacement[i])||next.cap_displacement[i]<0||next.cap_displacement[i]>1024)return 0;
        char key[16];snprintf(key,sizeof(key),"VISUAL%u",i);
        SA3DSPakChunk v;
        if(sa3dspak_find(&pak,key,&v)!=SA3DSPAK_OK||v.flags||
           sa3ds_ec_visual_open(&next.components[i],v.data,(size_t)v.size)!=SA3DS_EC_VISUAL_OK)return 0;
        SA3DSECVisual *view=&next.components[i];
        if(view->model_count!=1||view->instance_count!=1||view->vertex_count>2048||
           view->index_count>4096||view->material_count>16||view->textures.texture_count>16)return 0;
        size_t total=0;
        for(uint32_t j=0;j<view->textures.texture_count;j++){
            SA3TTexture t;const uint8_t *rgba;
            if(sa3t_texture(&view->textures,j,&t,&rgba)!=SA3T_OK||t.width<8||t.height<8||
               t.width>256||t.height>256||(t.width&(t.width-1))||(t.height&(t.height-1)))return 0;
            total+=t.data_size;
        }
        if(total>256u*1024u)return 0;
    }
    *out=next;return 1;
}
