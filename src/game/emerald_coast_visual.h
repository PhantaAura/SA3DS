#ifndef SA3DS_EMERALD_COAST_VISUAL_H
#define SA3DS_EMERALD_COAST_VISUAL_H

#include <stddef.h>
#include <stdint.h>
#include "sa3dspak.h"
#include "sa3t_texture.h"

typedef enum {
    SA3DS_EC_VISUAL_OK=0, SA3DS_EC_VISUAL_ERR_ARGUMENT,
    SA3DS_EC_VISUAL_ERR_PACKAGE, SA3DS_EC_VISUAL_ERR_MISSING_CHUNK,
    SA3DS_EC_VISUAL_ERR_FORMAT, SA3DS_EC_VISUAL_ERR_BOUNDS,
    SA3DS_EC_VISUAL_ERR_REFERENCE, SA3DS_EC_VISUAL_ERR_NONFINITE
} SA3DSECVisualResult;

typedef struct {
    uint32_t first_mesh,mesh_count,first_vertex,vertex_count,first_index,index_count;
    float center[3],radius;
    int32_t original_texture_id;
} SA3DSECModel;
typedef struct {
    uint16_t model_index,material_index;
    uint32_t first_index,index_count;
    uint16_t primitive,type_material;
    uint32_t polygon_count,flags;
} SA3DSECMesh;
typedef struct {
    uint32_t model_index,collision_index,object_flags,surface_flags;
    float position[3],rotation[3],scale[3],center[3],radius;
} SA3DSECInstance;
typedef struct { uint32_t diffuse,specular; float exponent; uint32_t texture_id,flags; } SA3DSECMaterial;
typedef struct { float position[3],normal[3],uv[2]; } SA3DSECVertex;

typedef struct {
    SA3DSPak package;
    const uint8_t *geometry,*materials;
    size_t geometry_size,materials_size;
    uint32_t model_count,mesh_count,instance_count,vertex_count,index_count,material_count;
    uint32_t model_offset,mesh_offset,instance_offset,vertex_offset,index_offset,material_offset;
    uint32_t material_stride;
    SA3TArchive textures;
} SA3DSECVisual;

SA3DSECVisualResult sa3ds_ec_visual_open(SA3DSECVisual *out,const void *data,size_t size);
int sa3ds_ec_model(const SA3DSECVisual *v,uint32_t i,SA3DSECModel *out);
int sa3ds_ec_mesh(const SA3DSECVisual *v,uint32_t i,SA3DSECMesh *out);
int sa3ds_ec_instance(const SA3DSECVisual *v,uint32_t i,SA3DSECInstance *out);
/* PC2004 0x43AA5B: signed test of COL+0x20; nonnegative entries never
 * enter the land draw list. Solid bit 0 is independent of visibility. */
int sa3ds_ec_instance_is_visible(const SA3DSECInstance *instance);
int sa3ds_ec_material(const SA3DSECVisual *v,uint32_t i,SA3DSECMaterial *out);
int sa3ds_ec_vertex(const SA3DSECVisual *v,uint32_t i,SA3DSECVertex *out);
int sa3ds_ec_index(const SA3DSECVisual *v,uint32_t i,uint16_t *out);
const char *sa3ds_ec_visual_result_name(SA3DSECVisualResult r);

#endif
