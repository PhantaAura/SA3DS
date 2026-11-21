#ifndef SA3DS_0F_LAB_GPU_H
#define SA3DS_0F_LAB_GPU_H

#include <3ds.h>
#include <citro3d.h>
#include <stdint.h>

#include "../../src/sa1/physics/player_state.h"

typedef struct {
    float position[3];
    float texcoord[2];
} SA3DSLabVertex;

typedef struct {
    SA3DSLabVertex *scratch;
    uint32_t scratch_capacity;
    uint32_t frame_draws;
    uint32_t frame_triangles;
} SA3DSLabGpu;

int sa3ds_lab_gpu_init(SA3DSLabGpu *lab);
void sa3ds_lab_gpu_destroy(SA3DSLabGpu *lab);
int sa3ds_lab_gpu_draw(SA3DSLabGpu *lab, const SA1PlayerState *player,
                       float camera_yaw);

#endif
