#include "emerald_coast_gpu.h"
#include "sa3c_gpu_preflight.h"
#include "sa3c_pose.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int selected(const SA3DSECInstance *x,float radius){
    return sa3ds_ec_instance_is_visible(x) &&
        hypotf(x->center[0],x->center[2])-x->radius<=radius;
}
static void transform(const SA3CMatrix*m,const float p[3],float out[3]){
    out[0]=p[0]*m->m[0]+p[1]*m->m[4]+p[2]*m->m[8]+m->m[12];
    out[1]=p[0]*m->m[1]+p[1]*m->m[5]+p[2]*m->m[9]+m->m[13];
    out[2]=p[0]*m->m[2]+p[1]*m->m[6]+p[2]*m->m[10]+m->m[14];
}

void sa3ds_ec_gpu_destroy(SA3DSECGpu*g){
    if(!g)return;
    if(g->textures){
        for(uint32_t i=0;i<g->texture_count;i++)
            if(g->texture_loaded&&g->texture_loaded[i]&&g->textures[i].data)
                C3D_TexDelete(&g->textures[i]);
        free(g->textures);
    }
    free(g->texture_loaded);free(g->draws);
    if(g->vertices)linearFree(g->vertices);
    memset(g,0,sizeof(*g));
}

int sa3ds_ec_gpu_init(SA3DSECGpu*g,const SA3DSECVisual*v,float radius,
    SA3DSSonicTextureTrace trace,void*userdata){
    if(!g||!v||radius<=0.0f||v->material_count>UINT32_MAX/2u||
       v->material_count>SIZE_MAX/(2u*sizeof(uint32_t)))return 0;
    memset(g,0,sizeof(*g));g->visual=v;g->subset_radius=radius;g->texture_count=v->textures.texture_count;
    /* One material can occur in both original land draw classes. Never merge
     * their vertices: a water instance must not change an opaque one's depth. */
    uint32_t buckets=v->material_count*2u;
    uint32_t *counts=(uint32_t*)calloc(buckets,sizeof(*counts));uint32_t *cursor=(uint32_t*)calloc(buckets,sizeof(*cursor));
    if(!counts||!cursor)goto fail;
    for(uint32_t ii=0;ii<v->instance_count;ii++){
        SA3DSECInstance in;SA3DSECModel model;
        if(!sa3ds_ec_instance(v,ii,&in)||!selected(&in,radius)||!sa3ds_ec_model(v,in.model_index,&model))continue;
        uint32_t water=(uint32_t)sa3ds_ec_instance_is_deferred_water(&in);
        g->instance_count++;
        for(uint32_t j=0;j<model.mesh_count;j++){
            SA3DSECMesh mesh;SA3DSECMaterial mat;
            if(!sa3ds_ec_mesh(v,model.first_mesh+j,&mesh)||
               !sa3ds_ec_material(v,mesh.material_index,&mat))goto fail;
            /* Unsupported blend factors fail safely instead of silently
             * assigning guessed water semantics to another source material. */
            if(water&&!sa3ds_ec_material_has_water_blend(&mat))goto fail;
            uint32_t bucket=mesh.material_index+water*v->material_count;
            if(UINT32_MAX-counts[bucket]<mesh.index_count)goto fail;
            counts[bucket]+=mesh.index_count;
        }
    }
    for(uint32_t i=0;i<buckets;i++){if(counts[i]){g->draw_count++;if(UINT32_MAX-g->vertex_count<counts[i])goto fail;g->vertex_count+=counts[i];}}
    if(!g->vertex_count||!g->draw_count||g->vertex_count>SIZE_MAX/sizeof(*g->vertices))goto fail;
    g->vertices=(SA3DSGpuVertex*)linearAlloc((size_t)g->vertex_count*sizeof(*g->vertices));g->draws=(SA3DSECDraw*)calloc(g->draw_count,sizeof(*g->draws));
    g->textures=(C3D_Tex*)calloc(g->texture_count,sizeof(*g->textures));g->texture_loaded=(uint8_t*)calloc(g->texture_count,1);
    if(!g->vertices||!g->draws||!g->textures||!g->texture_loaded)goto fail;
    uint32_t at=0,di=0;for(uint32_t i=0;i<buckets;i++)if(counts[i]){g->draws[di]=(SA3DSECDraw){at,counts[i],i%v->material_count,(uint8_t)(i/v->material_count)};cursor[i]=at;at+=counts[i];di++;}
    for(uint32_t ii=0;ii<v->instance_count;ii++){SA3DSECInstance in;SA3DSECModel model;if(!sa3ds_ec_instance(v,ii,&in)||!selected(&in,radius)||!sa3ds_ec_model(v,in.model_index,&model))continue;SA3CMatrix world;sa3c_matrix_build_object(&world,(SA3CVec3){in.position[0],in.position[1],in.position[2]},(SA3CVec3){in.rotation[0],in.rotation[1],in.rotation[2]},(SA3CVec3){in.scale[0],in.scale[1],in.scale[2]},in.object_flags);
        uint32_t water=(uint32_t)sa3ds_ec_instance_is_deferred_water(&in);
        for(uint32_t j=0;j<model.mesh_count;j++){SA3DSECMesh mesh;if(!sa3ds_ec_mesh(v,model.first_mesh+j,&mesh))goto fail;uint32_t bucket=mesh.material_index+water*v->material_count;for(uint32_t k=0;k<mesh.index_count;k++){uint16_t vi;SA3DSECVertex src;if(!sa3ds_ec_index(v,mesh.first_index+k,&vi)||!sa3ds_ec_vertex(v,vi,&src))goto fail;SA3DSGpuVertex*dst=&g->vertices[cursor[bucket]++];transform(&world,src.position,dst->position);dst->texcoord[0]=src.uv[0];dst->texcoord[1]=src.uv[1];}}
    }
    for(uint32_t i=0;i<g->draw_count;i++){SA3DSECMaterial mat;SA3TTexture td;const uint8_t*rgba=0;if(!sa3ds_ec_material(v,g->draws[i].material_index,&mat)||mat.texture_id>=g->texture_count)goto fail;if(g->texture_loaded[mat.texture_id])continue;if(sa3t_texture(&v->textures,mat.texture_id,&td,&rgba)!=SA3T_OK)goto fail;if(!sa3ds_upload_rgba8_cpu(&g->textures[mat.texture_id],td.width,td.height,rgba,mat.texture_id,g->texture_count,trace,userdata))goto fail;g->texture_loaded[mat.texture_id]=1;g->resident_texture_count++;g->texture_bytes+=td.data_size;}
    GSPGPU_FlushDataCache(g->vertices,(size_t)g->vertex_count*sizeof(*g->vertices));g->triangle_count=g->vertex_count/3u;free(cursor);free(counts);return 1;
fail: free(cursor);free(counts);sa3ds_ec_gpu_destroy(g);return 0;
}

int sa3ds_ec_gpu_draw(SA3DSECGpu*g,int deferred_water){
    if(!g||!g->vertices||(deferred_water!=0&&deferred_water!=1))return 0;
    int ok=1;
    /* PC2004 COL bit 1 -> queue parameter 0 -> ZWRITEENABLE=0. Keep depth
     * testing so foreground terrain/Sonic correctly mask the water. */
    C3D_DepthTest(true,GPU_GREATER,deferred_water?GPU_WRITE_COLOR:GPU_WRITE_ALL);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,
        GPU_ONE_MINUS_SRC_ALPHA,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA);
    for(uint32_t i=0;i<g->draw_count;i++){
        SA3DSECDraw*d=&g->draws[i];SA3DSECMaterial m;
        if(d->deferred_water!=(uint8_t)deferred_water)continue;
        if(!sa3ds_ec_material(g->visual,d->material_index,&m)||
           m.texture_id>=g->texture_count||!g->texture_loaded[m.texture_id]){ok=0;break;}
        C3D_BufInfo*buf=C3D_GetBufInfo();BufInfo_Init(buf);
        BufInfo_Add(buf,g->vertices+d->first_vertex,sizeof(*g->vertices),2,0x10);
        C3D_TexBind(0,&g->textures[m.texture_id]);C3D_TexEnv*env=C3D_GetTexEnv(0);
        C3D_TexEnvInit(env);C3D_TexEnvSrc(env,C3D_Both,GPU_TEXTURE0,GPU_CONSTANT,0);
        C3D_TexEnvFunc(env,C3D_Both,GPU_MODULATE);
        C3D_TexEnvColor(env,sa3ds_visual_material_rgba(m.diffuse));
        C3D_DrawArrays(GPU_TRIANGLES,0,d->vertex_count);
    }
    /* Do not leak deferred-water state into the character/next frame. */
    C3D_DepthTest(true,GPU_GREATER,GPU_WRITE_ALL);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,
        GPU_ONE_MINUS_SRC_ALPHA,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA);
    return ok;
}
