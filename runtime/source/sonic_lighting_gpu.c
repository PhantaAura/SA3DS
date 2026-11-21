#include "sonic_lighting_gpu.h"
#include "sonic_lit_shbin.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
/*No texture duplication. Both pending passes retain their colors until sync.*/
typedef struct {float position[3],uv[2],diffuse[4],specular[4];} LitVertex;
struct SA3DSSonicLightingGpu {
 DVLB_s *dvlb;shaderProgram_s program;int projection,view;LitVertex *vertices;
 uint32_t capacity;SA3DSLightingAssets assets;
};
static uint16_t r16(const uint8_t*p){return p[0]|((uint16_t)p[1]<<8);}
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static void point(const SA3CMatrix*m,const float*p,float*out){for(unsigned k=0;k<3;k++)out[k]=m->m[k]*p[0]+m->m[4+k]*p[1]+m->m[8+k]*p[2]+m->m[12+k];}
/*Inverse transpose of the complete actor * animated node transform. D3D8
 * normal normalization is disabled unless explicitly requested by the caller.*/
SA3DSSonicLightingGpu *sa3ds_sonic_lighting_gpu_create(const SA3DSSonicGpu*r,const SA3DSLightingAssets*a){
 if(!r||!a||!r->scratch_capacity)return NULL;
 SA3DSSonicLightingGpu*l=calloc(1,sizeof(*l));if(!l)return NULL;
 l->assets=*a;l->capacity=r->scratch_capacity;
 l->dvlb=DVLB_ParseFile((u32*)sonic_lit_shbin,sonic_lit_shbin_size);if(!l->dvlb)goto fail;
 shaderProgramInit(&l->program);shaderProgramSetVsh(&l->program,&l->dvlb->DVLE[0]);
 l->projection=shaderInstanceGetUniformLocation(l->program.vertexShader,"projection");
 l->view=shaderInstanceGetUniformLocation(l->program.vertexShader,"modelView");
 if(l->projection<0||l->view<0)goto fail;
 l->vertices=linearAlloc((size_t)l->capacity*sizeof(*l->vertices));if(!l->vertices)goto fail;
 return l;
fail:sa3ds_sonic_lighting_gpu_destroy(l);return NULL;
}
void sa3ds_sonic_lighting_gpu_destroy(SA3DSSonicLightingGpu*l){
 if(!l)return;
 if(l->vertices)linearFree(l->vertices);
 if(l->dvlb){shaderProgramFree(&l->program);DVLB_Free(l->dvlb);}free(l);
}
typedef struct {SA3DSSonicLightingGpu*l;SA3DSSonicGpu*r;const SA3CMatrix*actor;const float*eye;SA3DSLight8 lights[4];uint8_t mask;} Draw;
static int emit(const SA3CDrawCommand*cmd,void*opaque){
 Draw*d=opaque;SA3DSSonicGpu*r=d->r;const SA3CCharacter*c=r->character;
 if(!cmd->index_count)return 0;
 if(cmd->texture_slot>=r->texture_count||cmd->material_index>=c->sections[5].count)return 1;
 uint32_t first;r->last_batch_error=sa3ds_gpu_batch_reserve(&r->batch,cmd->index_count,&first);
 if(r->last_batch_error!=SA3DS_BATCH_OK||first+cmd->index_count>d->l->capacity)return 1;
 LitVertex*dst=d->l->vertices+first;const SA3CSection*vs=&c->sections[3],*is=&c->sections[4],*ms=&c->sections[5];
 const uint8_t*mat=c->data+ms->offset+(size_t)cmd->material_index*ms->stride;
 SA3DSMaterial8 material;if(!sa3ds_pc2004_basic_material8(0xffffffff,r32(mat+4),rf(mat+8),&material))return 1;
 float nm[9];if(!sa3ds_pc2004_normal_matrix(d->actor->m,cmd->world.m,nm))return 1;
 const float white[4]={1,1,1,1},ambient[3]={0,0,0};
 int highlight=0;int alpha=(r32(mat+16)&0x00100000u)!=0;
 for(uint32_t i=0;i<cmd->index_count;i++){
  uint16_t index=r16(c->data+is->offset+(size_t)(cmd->first_index+i)*2);if(index>=vs->count)return 1;
  const uint8_t*v=c->data+vs->offset+(size_t)index*vs->stride;
  float raw[3]={rf(v),rf(v+4),rf(v+8)},p[3],n[3],normal[3]={rf(v+12),rf(v+16),rf(v+20)};
  point(&cmd->world,raw,p);point(d->actor,p,dst[i].position);
  for(unsigned k=0;k<3;k++)n[k]=nm[k]*normal[0]+nm[3+k]*normal[1]+nm[6+k]*normal[2];
  if(!sa3ds_pc2004_light_vertex(d->lights,d->mask,&material,white,ambient,n,dst[i].position,d->eye,0,dst[i].diffuse,dst[i].specular))return 1;
  if(r32(mat+16)&0x02000000u){memcpy(dst[i].diffuse,white,sizeof(white));memset(dst[i].specular,0,sizeof(dst[i].specular));}
  for(unsigned k=0;k<3;k++)if(dst[i].specular[k]>0)highlight=1;
  dst[i].uv[0]=sa3c_basic_uv_normalize(rf(v+24));float vv=sa3c_basic_uv_normalize(rf(v+28));dst[i].uv[1]=r->v_flip_debug?-vv:vv;
 }
 GSPGPU_FlushDataCache(dst,(size_t)cmd->index_count*sizeof(*dst));
 C3D_TexBind(0,&r->gpu_textures[cmd->texture_slot]);
 C3D_BufInfo*b=C3D_GetBufInfo();BufInfo_Init(b);
 BufInfo_Add(b,dst,sizeof(*dst),2,0x10);BufInfo_Add(b,dst->diffuse,sizeof(*dst),1,2);
 C3D_TexEnv*e=C3D_GetTexEnv(0);C3D_TexEnvInit(e);
 C3D_TexEnvSrc(e,C3D_Both,GPU_TEXTURE0,GPU_PRIMARY_COLOR,0);C3D_TexEnvFunc(e,C3D_Both,GPU_MODULATE);
 C3D_DepthTest(true,GPU_GREATER,GPU_WRITE_ALL);
 C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,alpha?GPU_SRC_ALPHA:GPU_ONE,alpha?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ONE,GPU_ZERO);
 C3D_DrawArrays(GPU_TRIANGLES,0,cmd->index_count);r->frame_draws++;
 if(highlight){
  /*Gouraud specular is added after texturing; texture alpha still clips holes.
   *Depth equality retains the diffuse surface and leaves depth unchanged.*/
  b=C3D_GetBufInfo();BufInfo_Init(b);BufInfo_Add(b,dst,sizeof(*dst),2,0x10);BufInfo_Add(b,dst->specular,sizeof(*dst),1,2);
  e=C3D_GetTexEnv(0);C3D_TexEnvInit(e);
  C3D_TexEnvSrc(e,C3D_RGB,GPU_PRIMARY_COLOR,0,0);C3D_TexEnvFunc(e,C3D_RGB,GPU_REPLACE);
  C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE0,0,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_REPLACE);
  C3D_DepthTest(true,GPU_EQUAL,GPU_WRITE_COLOR);
  C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,alpha?GPU_SRC_ALPHA:GPU_ONE,GPU_ONE,GPU_ZERO,GPU_ONE);
  C3D_DrawArrays(GPU_TRIANGLES,0,cmd->index_count);r->frame_draws++;
  r->frame_vertices+=cmd->index_count;r->frame_triangles+=cmd->index_count/3;
 }
 r->frame_vertices+=cmd->index_count;r->frame_triangles+=cmd->index_count/3;return 0;
}
int sa3ds_sonic_lighting_gpu_draw(SA3DSSonicLightingGpu*l,SA3DSSonicGpu*r,unsigned act,
 const C3D_Mtx*projection,const C3D_Mtx*view,const SA3CMatrix*actor,const float eye[3],
 uint16_t root,const SA3CMatrix*world,size_t count,SA3CRenderStats*stats){
 if(!l||!r||!actor||!eye)return 0;
 Draw d={.l=l,.r=r,.actor=actor,.eye=eye};
 if(!sa3ds_ec_character_lights(&l->assets,act,d.lights,&d.mask))return 0;
 sa3ds_sonic_gpu_begin_frame(r);C3D_BindProgram(&l->program);
 C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,l->projection,projection);C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,l->view,view);
 C3D_AttrInfo*a=C3D_GetAttrInfo();AttrInfo_Init(a);AttrInfo_AddLoader(a,0,GPU_FLOAT,3);AttrInfo_AddLoader(a,1,GPU_FLOAT,2);AttrInfo_AddLoader(a,2,GPU_FLOAT,4);
 SA3CRenderStats st;SA3CPoseResult result=sa3c_render_emit(r->character,root,world,count,emit,&d,&st);
 C3D_DepthTest(true,GPU_GREATER,GPU_WRITE_ALL);
 C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
 if(stats)*stats=st;
 return result==SA3C_POSE_OK;
}
