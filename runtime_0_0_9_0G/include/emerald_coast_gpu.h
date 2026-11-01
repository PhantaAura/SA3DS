#ifndef SA3DS_EMERALD_COAST_GPU_H
#define SA3DS_EMERALD_COAST_GPU_H

#include <3ds.h>
#include <citro3d.h>
#include <stdint.h>
#include "emerald_coast_visual.h"
#include "sonic_gpu.h"

typedef struct {
    uint32_t first_vertex,vertex_count,material_index;
    uint32_t first_span,span_count;
    uint8_t deferred_water;
} SA3DSECDraw;
typedef struct {
    uint32_t first_vertex,vertex_count;
    float center[3],radius;
} SA3DSECSpan;
typedef struct {
    const SA3DSECVisual *visual;
    SA3DSGpuVertex *vertices;
    uint32_t vertex_count;
    SA3DSECDraw *draws;
    uint32_t draw_count,instance_count,triangle_count;
    C3D_Tex *textures;
    uint8_t *texture_loaded;
    uint32_t texture_count,resident_texture_count,texture_bytes;
    float subset_radius;
    int full_act;
    SA3DSECSpan *spans;
    uint32_t span_count,submitted_triangles,submitted_draws;
    /* World-space camera basis; no visibility decision changes simulation. */
    float eye[3],forward[3],right[3],up[3],tan_x,tan_y,near_clip,far_clip;
    int view_valid;
} SA3DSECGpu;

int sa3ds_ec_gpu_init(SA3DSECGpu *gpu,const SA3DSECVisual *visual,float subset_radius,
    SA3DSSonicTextureTrace trace,void *userdata);
/* Full one-act residency. Caller measures the actual linear-memory budget;
 * acts are replaced sequentially, rather than loading the entire course. */
int sa3ds_ec_gpu_init_full(SA3DSECGpu *gpu,const SA3DSECVisual *visual,
    SA3DSSonicTextureTrace trace,void *userdata);
/* Same right-handed eye/target and vertical FOV as Mtx_LookAt/PerspTilt.
 * Invalid views conservatively disable culling. Only full-act mode culls. */
void sa3ds_ec_gpu_set_view(SA3DSECGpu *gpu,const float eye[3],const float target[3],
    float fovy,float aspect,float near_clip,float far_clip);
/* Pass 0 before Sonic, pass 1 after Sonic, with the stage modelview bound.
 * Bounded water treatment, not the complete original sorted draw queue. */
int sa3ds_ec_gpu_draw(SA3DSECGpu *gpu,int deferred_water);
void sa3ds_ec_gpu_destroy(SA3DSECGpu *gpu);

#endif
