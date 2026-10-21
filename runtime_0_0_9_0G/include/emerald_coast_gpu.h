#ifndef SA3DS_EMERALD_COAST_GPU_H
#define SA3DS_EMERALD_COAST_GPU_H

#include <3ds.h>
#include <citro3d.h>
#include <stdint.h>
#include "emerald_coast_visual.h"
#include "sonic_gpu.h"

typedef struct { uint32_t first_vertex,vertex_count,material_index; } SA3DSECDraw;
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
} SA3DSECGpu;

int sa3ds_ec_gpu_init(SA3DSECGpu *gpu,const SA3DSECVisual *visual,float subset_radius,
    SA3DSSonicTextureTrace trace,void *userdata);
int sa3ds_ec_gpu_draw(SA3DSECGpu *gpu);
void sa3ds_ec_gpu_destroy(SA3DSECGpu *gpu);

#endif
