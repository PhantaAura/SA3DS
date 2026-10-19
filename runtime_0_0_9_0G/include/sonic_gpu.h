#ifndef SA3DS_0E_SONIC_GPU_H
#define SA3DS_0E_SONIC_GPU_H

#include <3ds.h>
#include <citro3d.h>
#include <stddef.h>
#include <stdint.h>
#include "sa3c_character.h"
#include "sa3c_pose.h"
#include "sa3c_render.h"
#include "sa3c_gpu_batch.h"
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
    SA3DSGpuBatch batch;
    SA3DSBatchResult last_batch_error;
    uint32_t frame_draws;
    uint32_t frame_triangles;
    uint32_t frame_vertices;
    int v_flip_debug;
} SA3DSSonicGpu;

/* Diagnostic events are emitted *before* each potentially blocking GPU step.
   callback is optional; no owner texture pixels or other payloads are logged. */
typedef void (*SA3DSSonicTextureTrace)(const char *phase, uint32_t texture_index,
    uint32_t texture_count, uint16_t width, uint16_t height, void *userdata);
int sa3ds_sonic_gpu_init_traced(SA3DSSonicGpu *r, const SA3CCharacter *c,
    const SA3TArchive *t, SA3DSSonicTextureTrace trace, void *userdata);
int sa3ds_sonic_gpu_init(SA3DSSonicGpu *r, const SA3CCharacter *c, const SA3TArchive *t);
void sa3ds_sonic_gpu_destroy(SA3DSSonicGpu *r);
int sa3ds_sonic_gpu_draw(SA3DSSonicGpu *r, uint16_t root_node, const SA3CMatrix *world, size_t world_count, SA3CRenderStats *out_stats);

#endif
