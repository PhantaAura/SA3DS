#ifndef SA3DS_0E_SONIC_GPU_H
#define SA3DS_0E_SONIC_GPU_H

#include <3ds.h>
#include <citro3d.h>
#include <stddef.h>
#include <stdint.h>
#include "sa3c_character.h"
#include "sa3c_pose.h"
#include "sa3c_render.h"
#include "sa3t_texture.h"

typedef struct {
    float position[3];
    float texcoord[2];
} SA3DSGpuVertex;

typedef struct {
    const SA3CCharacter *character;
    const SA3TArchive *textures;
    C3D_Tex *gpu_textures;
    uint32_t texture_count;
    SA3DSGpuVertex *scratch;
    uint32_t scratch_capacity;
    uint32_t frame_draws;
    uint32_t frame_triangles;
    uint32_t frame_vertices;
    int v_flip_debug;
} SA3DSSonicGpu;

int sa3ds_sonic_gpu_init(SA3DSSonicGpu *r, const SA3CCharacter *c, const SA3TArchive *t);
void sa3ds_sonic_gpu_destroy(SA3DSSonicGpu *r);
int sa3ds_sonic_gpu_draw(SA3DSSonicGpu *r, uint16_t root_node, const SA3CMatrix *world, size_t world_count, SA3CRenderStats *out_stats);

#endif
