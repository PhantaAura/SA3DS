#include "emerald_coast_visual.h"

int sa3ds_ec_instance_is_visible(const SA3DSECInstance *instance) {
    return instance && (instance->surface_flags & UINT32_C(0x80000000)) != 0;
}
int sa3ds_ec_instance_is_deferred_water(const SA3DSECInstance *instance) {
    return instance && (instance->surface_flags & UINT32_C(0x02000002)) == 2;
}
int sa3ds_ec_material_has_water_blend(const SA3DSECMaterial *material) {
    return material && (material->flags & UINT32_C(0x00200000)) &&
        (material->flags >> 29) == 4 && ((material->flags >> 26) & 7) == 5;
}
#include <math.h>
#include <string.h>

#define EC_VIS_HEADER 64u
#define EC_MODEL_SIZE 48u
#define EC_MESH_SIZE 24u
#define EC_INSTANCE_SIZE 84u
#define EC_VERTEX_SIZE 32u
#define EC_MAT_HEADER 32u
#define EC_MAT_SIZE 20u

static uint16_t r16(const uint8_t*p){return (uint16_t)p[0]|(uint16_t)((uint16_t)p[1]<<8);}
static uint32_t r32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static int span(uint32_t off,uint32_t count,uint32_t stride,size_t size){
    return off<=size && count<=((size-off)/stride);
}
static int finite3(const float a[3]){return isfinite(a[0])&&isfinite(a[1])&&isfinite(a[2]);}

int sa3ds_ec_model(const SA3DSECVisual*v,uint32_t i,SA3DSECModel*out){
    if(!v||!out||i>=v->model_count)return 0;
    const uint8_t*p=v->geometry+v->model_offset+(size_t)i*EC_MODEL_SIZE;
    out->first_mesh=r32(p);out->mesh_count=r32(p+4);out->first_vertex=r32(p+8);out->vertex_count=r32(p+12);
    out->first_index=r32(p+16);out->index_count=r32(p+20);for(int n=0;n<3;n++)out->center[n]=rf(p+24+n*4);
    out->radius=rf(p+36);out->original_texture_id=(int32_t)r32(p+40);return 1;
}
int sa3ds_ec_mesh(const SA3DSECVisual*v,uint32_t i,SA3DSECMesh*out){
    if(!v||!out||i>=v->mesh_count)return 0;
    const uint8_t*p=v->geometry+v->mesh_offset+(size_t)i*EC_MESH_SIZE;
    out->model_index=r16(p);out->material_index=r16(p+2);out->first_index=r32(p+4);out->index_count=r32(p+8);
    out->primitive=r16(p+12);out->type_material=r16(p+14);out->polygon_count=r32(p+16);out->flags=r32(p+20);return 1;
}
int sa3ds_ec_instance(const SA3DSECVisual*v,uint32_t i,SA3DSECInstance*out){
    if(!v||!out||i>=v->instance_count)return 0;
    const uint8_t*p=v->geometry+v->instance_offset+(size_t)i*EC_INSTANCE_SIZE;
    out->model_index=r32(p);out->collision_index=r32(p+4);out->object_flags=r32(p+8);out->surface_flags=r32(p+12);
    for(int n=0;n<3;n++){out->position[n]=rf(p+32+n*4);out->rotation[n]=rf(p+44+n*4);out->scale[n]=rf(p+56+n*4);out->center[n]=rf(p+68+n*4);}out->radius=rf(p+80);return 1;
}
int sa3ds_ec_material(const SA3DSECVisual*v,uint32_t i,SA3DSECMaterial*out){
    if(!v||!out||i>=v->material_count)return 0;
    const uint8_t*p=v->materials+v->material_offset+(size_t)i*v->material_stride;
    out->diffuse=r32(p);out->specular=r32(p+4);out->exponent=rf(p+8);out->texture_id=r32(p+12);out->flags=r32(p+16);return 1;
}
int sa3ds_ec_vertex(const SA3DSECVisual*v,uint32_t i,SA3DSECVertex*out){
    if(!v||!out||i>=v->vertex_count)return 0;
    const uint8_t*p=v->geometry+v->vertex_offset+(size_t)i*EC_VERTEX_SIZE;
    for(int n=0;n<3;n++){out->position[n]=rf(p+n*4);out->normal[n]=rf(p+12+n*4);}out->uv[0]=rf(p+24);out->uv[1]=rf(p+28);return 1;
}
int sa3ds_ec_index(const SA3DSECVisual*v,uint32_t i,uint16_t*out){if(!v||!out||i>=v->index_count)return 0;*out=r16(v->geometry+v->index_offset+(size_t)i*2u);return 1;}

SA3DSECVisualResult sa3ds_ec_visual_open(SA3DSECVisual*out,const void*data,size_t size){
    if(!out||!data)return SA3DS_EC_VISUAL_ERR_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(sa3dspak_open(&out->package,data,size)!=SA3DSPAK_OK)return SA3DS_EC_VISUAL_ERR_PACKAGE;
    SA3DSPakChunk metadata,g,m,t;
    if(out->package.toc_count!=4||
       sa3dspak_find(&out->package,"METADATA",&metadata)!=SA3DSPAK_OK||
       sa3dspak_find(&out->package,"GEOMETRY",&g)!=SA3DSPAK_OK||
       sa3dspak_find(&out->package,"MATERIALS",&m)!=SA3DSPAK_OK||
       sa3dspak_find(&out->package,"TEXTURES",&t)!=SA3DSPAK_OK)
        return SA3DS_EC_VISUAL_ERR_MISSING_CHUNK;
    if(g.size<EC_VIS_HEADER||memcmp(g.data,"SA3DVIS\0",8)||r16(g.data+8)!=1||r16(g.data+10)!=0)return SA3DS_EC_VISUAL_ERR_FORMAT;
    out->geometry=g.data;out->geometry_size=(size_t)g.size;out->model_count=r32(g.data+12);out->mesh_count=r32(g.data+16);out->instance_count=r32(g.data+20);out->vertex_count=r32(g.data+24);out->index_count=r32(g.data+28);
    out->model_offset=r32(g.data+32);out->mesh_offset=r32(g.data+36);out->instance_offset=r32(g.data+40);out->vertex_offset=r32(g.data+44);out->index_offset=r32(g.data+48);
    if(!out->model_count||!out->mesh_count||!out->instance_count||!out->vertex_count||!out->index_count||
       !span(out->model_offset,out->model_count,EC_MODEL_SIZE,g.size)||
       !span(out->mesh_offset,out->mesh_count,EC_MESH_SIZE,g.size)||
       !span(out->instance_offset,out->instance_count,EC_INSTANCE_SIZE,g.size)||
       !span(out->vertex_offset,out->vertex_count,EC_VERTEX_SIZE,g.size)||
       !span(out->index_offset,out->index_count,2,g.size)||
       out->model_offset<EC_VIS_HEADER||
       out->model_offset+(uint64_t)out->model_count*EC_MODEL_SIZE>out->mesh_offset||
       out->mesh_offset+(uint64_t)out->mesh_count*EC_MESH_SIZE>out->instance_offset||
       out->instance_offset+(uint64_t)out->instance_count*EC_INSTANCE_SIZE>out->vertex_offset||
       out->vertex_offset+(uint64_t)out->vertex_count*EC_VERTEX_SIZE>out->index_offset||
       out->index_offset+(uint64_t)out->index_count*2u!=g.size)
        return SA3DS_EC_VISUAL_ERR_BOUNDS;
    if(m.size<EC_MAT_HEADER||memcmp(m.data,"SA3DMATL",8)||r16(m.data+8)!=1||r16(m.data+10)!=0)return SA3DS_EC_VISUAL_ERR_FORMAT;
    out->materials=m.data;out->materials_size=(size_t)m.size;out->material_count=r32(m.data+12);out->material_stride=r32(m.data+16);out->material_offset=r32(m.data+20);
    if(out->material_stride!=EC_MAT_SIZE||r32(m.data+24)!=m.size||!span(out->material_offset,out->material_count,out->material_stride,m.size))return SA3DS_EC_VISUAL_ERR_BOUNDS;
    if(sa3t_open(&out->textures,t.data,(size_t)t.size)!=SA3T_OK)return SA3DS_EC_VISUAL_ERR_FORMAT;
    for(uint32_t i=0;i<out->model_count;i++){SA3DSECModel x;sa3ds_ec_model(out,i,&x);if(x.first_mesh>out->mesh_count||x.mesh_count>out->mesh_count-x.first_mesh||x.first_vertex>out->vertex_count||x.vertex_count>out->vertex_count-x.first_vertex||x.first_index>out->index_count||x.index_count>out->index_count-x.first_index)return SA3DS_EC_VISUAL_ERR_REFERENCE;if(!finite3(x.center)||!isfinite(x.radius)||x.radius<0)return SA3DS_EC_VISUAL_ERR_NONFINITE;}
    for(uint32_t i=0;i<out->mesh_count;i++){SA3DSECMesh x;sa3ds_ec_mesh(out,i,&x);if(x.model_index>=out->model_count||x.material_index>=out->material_count||x.first_index>out->index_count||x.index_count>out->index_count-x.first_index||(x.index_count%3u))return SA3DS_EC_VISUAL_ERR_REFERENCE;}
    for(uint32_t i=0;i<out->instance_count;i++){SA3DSECInstance x;sa3ds_ec_instance(out,i,&x);if(x.model_index>=out->model_count)return SA3DS_EC_VISUAL_ERR_REFERENCE;if(!finite3(x.position)||!finite3(x.rotation)||!finite3(x.scale)||!finite3(x.center)||!isfinite(x.radius)||x.radius<0)return SA3DS_EC_VISUAL_ERR_NONFINITE;}
    for(uint32_t i=0;i<out->material_count;i++){SA3DSECMaterial x;sa3ds_ec_material(out,i,&x);if(x.texture_id>=out->textures.texture_count)return SA3DS_EC_VISUAL_ERR_REFERENCE;if(!isfinite(x.exponent))return SA3DS_EC_VISUAL_ERR_NONFINITE;}
    for(uint32_t i=0;i<out->vertex_count;i++){SA3DSECVertex x;sa3ds_ec_vertex(out,i,&x);if(!finite3(x.position)||!finite3(x.normal)||!isfinite(x.uv[0])||!isfinite(x.uv[1]))return SA3DS_EC_VISUAL_ERR_NONFINITE;}
    for(uint32_t i=0;i<out->index_count;i++){uint16_t x;sa3ds_ec_index(out,i,&x);if(x>=out->vertex_count)return SA3DS_EC_VISUAL_ERR_REFERENCE;}
    return SA3DS_EC_VISUAL_OK;
}
const char*sa3ds_ec_visual_result_name(SA3DSECVisualResult r){static const char*n[]={"ok","invalid argument","invalid package","missing chunk","invalid format","out of bounds","invalid reference","nonfinite value"};return (unsigned)r<sizeof(n)/sizeof(n[0])?n[r]:"unknown";}
