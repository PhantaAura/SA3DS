/* Runs the ACTUAL native stage GPU batching/draw code at a mocked SDK boundary.
 * Synthetic source-neutral data only; this is not a rasterization oracle. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emerald_coast_gpu.h"

static C3D_BufInfo buffer;
static C3D_TexEnv environment;
static int allocations,uploads,deletes,draw_count,depth_mask,blend[6];
static const void *draw_vertices[2];
static int expected_mask;

void *linearAlloc(size_t size){void *p=malloc(size);if(p)allocations++;return p;}
void linearFree(void *p){if(p){allocations--;free(p);}}
void GSPGPU_FlushDataCache(const void *p,size_t size){assert(p&&size);}
void C3D_TexDelete(C3D_Tex *t){assert(t->data);free(t->data);t->data=NULL;deletes++;}
C3D_BufInfo *C3D_GetBufInfo(void){return &buffer;}
void BufInfo_Init(C3D_BufInfo *p){p->data=NULL;}
int BufInfo_Add(C3D_BufInfo *p,const void *data,ptrdiff_t stride,int count,uint64_t permutation){
    assert(stride==(ptrdiff_t)sizeof(SA3DSGpuVertex)&&count==2&&permutation==0x10);
    p->data=data;return 0;
}
void C3D_TexBind(int unit,C3D_Tex *t){assert(unit==0&&t->data);}
C3D_TexEnv *C3D_GetTexEnv(int unit){assert(unit==0);return &environment;}
void C3D_TexEnvInit(C3D_TexEnv *p){p->color=0;}
void C3D_TexEnvSrc(C3D_TexEnv *p,int mode,int a,int b,int c){
    assert(p==&environment&&mode==C3D_Both&&a==GPU_TEXTURE0&&b==GPU_CONSTANT&&c==0);
}
void C3D_TexEnvFunc(C3D_TexEnv *p,int mode,int function){assert(p==&environment&&mode==C3D_Both&&function==GPU_MODULATE);}
void C3D_TexEnvColor(C3D_TexEnv *p,uint32_t color){p->color=color;}
void C3D_DepthTest(bool enabled,int function,int mask){assert(enabled&&function==GPU_GREATER);depth_mask=mask;}
void C3D_AlphaBlend(int a,int b,int c,int d,int e,int f){
    blend[0]=a;blend[1]=b;blend[2]=c;blend[3]=d;blend[4]=e;blend[5]=f;
}
static void check_blend(void){
    assert(blend[0]==GPU_BLEND_ADD&&blend[1]==GPU_BLEND_ADD);
    assert(blend[2]==GPU_SRC_ALPHA&&blend[3]==GPU_ONE_MINUS_SRC_ALPHA);
    assert(blend[4]==GPU_SRC_ALPHA&&blend[5]==GPU_ONE_MINUS_SRC_ALPHA);
}
void C3D_DrawArrays(int primitive,int first,int count){
    assert(primitive==GPU_TRIANGLES&&first==0&&count==3);
    assert(depth_mask==expected_mask);check_blend();
    assert(draw_count<2);draw_vertices[draw_count++]=buffer.data;
}
int sa3ds_upload_rgba8_cpu(C3D_Tex *t,uint16_t width,uint16_t height,
    const uint8_t *rgba,uint32_t index,uint32_t total,
    SA3DSSonicTextureTrace trace,void *userdata){
    (void)trace;(void)userdata;
    assert(width==8&&height==8&&rgba&&index==0&&total==1);
    t->data=malloc(1);assert(t->data);uploads++;return 1;
}
static void put32(unsigned char *p,uint32_t u){
    p[0]=(unsigned char)u;p[1]=(unsigned char)(u>>8);p[2]=(unsigned char)(u>>16);p[3]=(unsigned char)(u>>24);
}
int main(int argc,char **argv){
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fseek(f,0,SEEK_END)==0);long length=ftell(f);assert(length>0);
    rewind(f);unsigned char *data=malloc((size_t)length);assert(data);
    assert(fread(data,1,(size_t)length,f)==(size_t)length);assert(fclose(f)==0);
    SA3DSECVisual v;assert(sa3ds_ec_visual_open(&v,data,(size_t)length)==SA3DS_EC_VISUAL_OK);
    /* Two instances share the SAME model/material but different land classes.
     * Extend the decoded fixture view; leave original vertex/index tables intact. */
    size_t original_size=v.geometry_size;
    unsigned char *geometry=malloc(original_size+168);assert(geometry);
    memcpy(geometry,v.geometry,original_size);
    memcpy(geometry+original_size,v.geometry+v.instance_offset,84);
    memcpy(geometry+original_size+84,v.geometry+v.instance_offset,84);
    put32(geometry+original_size+12,0x80000001u);
    put32(geometry+original_size+84+12,0x80000002u);
    float translated=10.0f;memcpy(geometry+original_size+84+32,&translated,4);
    v.geometry=geometry;v.geometry_size=original_size+168;
    v.instance_offset=(uint32_t)original_size;v.instance_count=2;
    unsigned char *material=(unsigned char*)v.materials+v.material_offset;
    put32(material+16,0x9438a400u);
    SA3DSECGpu gpu;assert(sa3ds_ec_gpu_init(&gpu,&v,500,NULL,NULL));
    assert(gpu.instance_count==2&&gpu.draw_count==2&&gpu.vertex_count==6&&gpu.triangle_count==2);
    assert(uploads==1&&gpu.resident_texture_count==1); /* no duplicated texture */
    assert(gpu.draws[0].material_index==0&&gpu.draws[1].material_index==0);
    assert(!gpu.draws[0].deferred_water&&gpu.draws[1].deferred_water);
    assert(gpu.draws[0].first_vertex==0&&gpu.draws[1].first_vertex==3);
    assert(gpu.vertices[0].position[0]==0&&gpu.vertices[3].position[0]==10);
    expected_mask=GPU_WRITE_ALL;assert(sa3ds_ec_gpu_draw(&gpu,0));
    assert(draw_count==1&&depth_mask==GPU_WRITE_ALL);check_blend();
    expected_mask=GPU_WRITE_COLOR;assert(sa3ds_ec_gpu_draw(&gpu,1));
    assert(draw_count==2&&draw_vertices[0]!=draw_vertices[1]);
    assert(depth_mask==GPU_WRITE_ALL);check_blend();
    gpu.texture_loaded[0]=0; /* failure AFTER deferred state is installed */
    assert(!sa3ds_ec_gpu_draw(&gpu,1));assert(draw_count==2);
    assert(depth_mask==GPU_WRITE_ALL);check_blend();
    gpu.texture_loaded[0]=1;
    assert(!sa3ds_ec_gpu_draw(&gpu,2));
    sa3ds_ec_gpu_destroy(&gpu);assert(allocations==0&&deletes==1);
    /* Unsupported blend factors must not submit guessed water treatment. */
    put32(material+16,0x9038a400u);
    assert(!sa3ds_ec_gpu_init(&gpu,&v,500,NULL,NULL));
    assert(allocations==0&&uploads==1&&gpu.vertices==NULL);
    free(geometry);free(data);
    puts("Emerald Coast actual GPU code / mocked SDK: PASS (shared-material class buckets, two phases, depth/blend restore, failure cleanup)");
    return 0;
}
