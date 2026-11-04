#include "whale_visual.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char**argv){
 if(argc==1){
  const uint8_t map[]={0,0,1,0,0,0};SA3DSWhaleVisual v={0};v.source_indices=map;v.visual.vertex_count=3;v.shape.vertices=2;uint16_t index;
  assert(sa3ds_whale_visual_source_index(&v,1,&index)&&index==1);
  assert(sa3ds_whale_visual_source_index(&v,2,&index)&&index==0);
  assert(!sa3ds_whale_visual_source_index(&v,3,&index));
  SA3DSWhaleVisual empty;assert(!sa3ds_whale_visual_open(&empty,NULL,0)&&!empty.source_indices);
  puts("Whale visual: PASS source identity lookup/bounds and failed-open reset; owner seam/mesh comparison separate");return 0;
 }
 FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);void*data=malloc(size);assert(data&&fread(data,1,size,f)==(size_t)size);fclose(f);
 SA3DSWhaleVisual v;int valid=sa3ds_whale_visual_open(&v,data,size);
 if(argc==2){free(data);return valid?1:0;}
 assert(valid);f=fopen(argv[2],"wb");assert(f);
 uint32_t counts[]={v.visual.vertex_count,v.visual.index_count,v.visual.mesh_count,v.visual.material_count};assert(fwrite(counts,4,4,f)==4);
 for(uint32_t i=0;i<v.visual.vertex_count;i++){
  uint16_t index;SA3DSECVertex vertex;assert(sa3ds_whale_visual_source_index(&v,i,&index)&&sa3ds_ec_vertex(&v.visual,i,&vertex));
  assert(fwrite(&index,2,1,f)==1&&fwrite(&vertex,32,1,f)==1);
 }
 for(uint32_t i=0;i<v.visual.index_count;i++){uint16_t index;assert(sa3ds_ec_index(&v.visual,i,&index)&&fwrite(&index,2,1,f)==1);}
 for(uint32_t i=0;i<v.visual.mesh_count;i++){SA3DSECMesh m;assert(sa3ds_ec_mesh(&v.visual,i,&m));uint32_t row[]={m.material_index,m.first_index,m.index_count};assert(fwrite(row,4,3,f)==3);}
 for(uint32_t i=0;i<v.visual.material_count;i++){SA3DSECMaterial m;assert(sa3ds_ec_material(&v.visual,i,&m)&&fwrite(&m,20,1,f)==1);}
 assert(!fclose(f));
 float(*positions)[3]=malloc(v.shape.vertices*12),(*normals)[3]=malloc(v.shape.vertices*12);SA3DSECVertex*vertices=malloc(v.visual.vertex_count*sizeof(*vertices));assert(positions&&normals&&vertices);
 for(unsigned bank=0;bank<2;bank++)for(unsigned tick=0;tick<=v.shape.banks[bank].duration*4;tick++){
  assert(sa3ds_whale_visual_sample(&v,bank,tick*.25f,positions,normals,v.shape.vertices,vertices,v.visual.vertex_count));
  for(uint32_t i=0;i<v.visual.vertex_count;i++){
   uint16_t index;SA3DSECVertex base;assert(sa3ds_whale_visual_source_index(&v,i,&index)&&sa3ds_ec_vertex(&v.visual,i,&base));
   assert(!memcmp(vertices[i].position,positions[index],12)&&!memcmp(vertices[i].normal,normals[index],12)&&!memcmp(vertices[i].uv,base.uv,8));
  }
 }
 assert(!sa3ds_whale_visual_sample(&v,0,0,positions,normals,v.shape.vertices,vertices,v.visual.vertex_count-1));
 free(positions);free(normals);free(vertices);free(data);puts("Whale visual owner runtime: PASS both-bank seam-preserving scatter and unchanged UVs");return 0;
}
