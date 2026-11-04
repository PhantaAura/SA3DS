#include "whale_visual.h"
#include <string.h>
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
int sa3ds_whale_visual_open(SA3DSWhaleVisual*out,const void*bytes,size_t size){
 if(!out)return 0;
 memset(out,0,sizeof(*out));SA3DSPak p;SA3DSPakChunk c,v,s,m;
 if(!bytes||size>8u*1024u*1024u||sa3dspak_open(&p,bytes,size)!=SA3DSPAK_OK||p.toc_count!=4||
    sa3dspak_find(&p,"METADATA",&m)!=SA3DSPAK_OK||m.flags||
    sa3dspak_find(&p,"WHALE_MAP",&c)!=SA3DSPAK_OK||c.flags||c.size<32||
    sa3dspak_find(&p,"WHALE_SHAPE",&s)!=SA3DSPAK_OK||s.flags||
    sa3dspak_find(&p,"VISUAL",&v)!=SA3DSPAK_OK||v.flags)return 0;
 SA3DSWhaleVisual next={0};const uint8_t*b=c.data;
 if(sa3ds_ec_visual_open(&next.visual,v.data,(size_t)v.size)!=SA3DS_EC_VISUAL_OK||
    !sa3ds_whale_shape_open(&next.shape,s.data,(size_t)s.size)||
    next.visual.model_count!=1||next.visual.instance_count!=1||next.visual.vertex_count>4096||next.visual.index_count>8192||
    memcmp(b,"SA3WMAP\0",8)||r32(b+8)!=1||r32(b+12)!=next.shape.vertices||
    r32(b+16)!=next.visual.vertex_count||r32(b+20)!=32||r32(b+24)!=2||r32(b+28)!=c.size||
    c.size!=32+next.visual.vertex_count*2)return 0;
 next.source_indices=b+32;
 for(uint32_t i=0;i<next.visual.vertex_count;i++){
  uint16_t index;if(!sa3ds_whale_visual_source_index(&next,i,&index))return 0;
 }
 *out=next;return 1;
}
int sa3ds_whale_visual_source_index(const SA3DSWhaleVisual*v,uint32_t vertex,uint16_t*out){
 if(!v||!v->source_indices||!out||vertex>=v->visual.vertex_count)return 0;
 const uint8_t*p=v->source_indices+vertex*2;uint16_t index=(uint16_t)(p[0]|((uint16_t)p[1]<<8));
 if(index>=v->shape.vertices)return 0;
 *out=index;return 1;
}
int sa3ds_whale_visual_sample(const SA3DSWhaleVisual*v,unsigned bank,float frame,
 float(*positions)[3],float(*normals)[3],size_t scratch_count,SA3DSECVertex*vertices,size_t capacity){
 if(!v||!vertices||capacity<v->visual.vertex_count||
    !sa3ds_whale_shape_sample(&v->shape,bank,frame,positions,normals,scratch_count))return 0;
 for(uint32_t i=0;i<v->visual.vertex_count;i++){
  uint16_t source;if(!sa3ds_whale_visual_source_index(v,i,&source)||!sa3ds_ec_vertex(&v->visual,i,&vertices[i]))return 0;
  memcpy(vertices[i].position,positions[source],sizeof(vertices[i].position));
  memcpy(vertices[i].normal,normals[source],sizeof(vertices[i].normal));
 }
 return 1;
}
