#include "whale_gpu.h"
#include <float.h>
#include <stdlib.h>
#include <string.h>
void sa3ds_whale_gpu_destroy(SA3DSWhaleGpu*g){
 if(!g)return;
 sa3ds_ec_gpu_destroy(&g->gpu);free(g->render_indices);free(g->sampled);free(g->positions);free(g->normals);memset(g,0,sizeof(*g));
}
int sa3ds_whale_gpu_init(SA3DSWhaleGpu*g,const SA3DSWhaleVisual*v,SA3DSSonicTextureTrace trace,void*userdata){
 if(!g||!v)return 0;
 memset(g,0,sizeof(*g));SA3DSECInstance instance;
 if(!sa3ds_ec_instance(&v->visual,0,&instance)||!sa3ds_ec_instance_is_visible(&instance)||sa3ds_ec_instance_is_deferred_water(&instance))return 0;
 /*The package holds one identity resource. Actor/root motion is applied by
  *the caller's modelview, not baked twice into dynamic source positions.*/
 for(unsigned k=0;k<3;k++)if(instance.position[k]!=0||instance.rotation[k]!=0||instance.scale[k]!=1)return 0;
 if(!sa3ds_ec_gpu_init(&g->gpu,&v->visual,FLT_MAX,trace,userdata))return 0;
 if(g->gpu.vertex_count!=v->visual.index_count)goto fail;
 g->visual=v;g->render_indices=malloc(g->gpu.vertex_count*sizeof(*g->render_indices));
 g->sampled=malloc(v->visual.vertex_count*sizeof(*g->sampled));
 g->positions=malloc(v->shape.vertices*sizeof(*g->positions));g->normals=malloc(v->shape.vertices*sizeof(*g->normals));
 if(!g->render_indices||!g->sampled||!g->positions||!g->normals)goto fail;
 /*EC's existing renderer packs pending triangles by material. Preserve that
  *exact packing while resolving every UV-split vertex through the source map.*/
 for(uint32_t draw=0;draw<g->gpu.draw_count;draw++){
  const SA3DSECDraw*d=&g->gpu.draws[draw];uint32_t cursor=d->first_vertex,end=cursor+d->vertex_count;
  for(uint32_t j=0;j<v->visual.mesh_count;j++){
   SA3DSECMesh mesh;if(!sa3ds_ec_mesh(&v->visual,j,&mesh))goto fail;
   if(mesh.material_index!=d->material_index)continue;
   for(uint32_t k=0;k<mesh.index_count;k++){
    uint16_t index;if(cursor>=end||!sa3ds_ec_index(&v->visual,mesh.first_index+k,&index))goto fail;
    g->render_indices[cursor++]=index;
   }
  }
  if(cursor!=end)goto fail;
 }
 return 1;
fail:sa3ds_whale_gpu_destroy(g);return 0;
}
int sa3ds_whale_gpu_update(SA3DSWhaleGpu*g,unsigned bank,float frame){
 if(!g||!g->visual||!g->render_indices||!sa3ds_whale_visual_sample(g->visual,bank,frame,
    g->positions,g->normals,g->visual->shape.vertices,g->sampled,g->visual->visual.vertex_count))return 0;
 for(uint32_t i=0;i<g->gpu.vertex_count;i++){
  const SA3DSECVertex*v=&g->sampled[g->render_indices[i]];
  memcpy(g->gpu.vertices[i].position,v->position,sizeof(v->position));
  /*UVs remain the original CPU-uploaded values; sampled normals are retained
   *for the future original lighting path, not substituted with flat normals.*/
 }
 GSPGPU_FlushDataCache(g->gpu.vertices,g->gpu.vertex_count*sizeof(*g->gpu.vertices));return 1;
}
int sa3ds_whale_gpu_draw(SA3DSWhaleGpu*g){return g&&g->visual&&sa3ds_ec_gpu_draw(&g->gpu,0);}
