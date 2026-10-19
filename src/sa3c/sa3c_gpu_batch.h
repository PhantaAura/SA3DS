#ifndef SA3DS_SA3C_GPU_BATCH_H
#define SA3DS_SA3C_GPU_BATCH_H

#include <stdint.h>
#include "sa3c_character.h"

/*
 * Citro3D draw commands may remain queued until FrameEnd. Every draw must
 * retain a distinct vertex span until the frame completes; reusing one
 * max-mesh-sized scratch buffer corrupts earlier pending draw submissions.
 *
 * The budget conservatively counts each model instance referenced by any
 * NODE, even if hidden by a particular action. It does not depend on a
 * specific animation root or redistribute Sega-owned geometry.
 */
typedef enum {
    SA3DS_BATCH_OK = 0,
    SA3DS_BATCH_ARGUMENT,
    SA3DS_BATCH_BAD_REFERENCE,
    SA3DS_BATCH_OVERFLOW,
    SA3DS_BATCH_EMPTY,
    SA3DS_BATCH_CAPACITY
} SA3DSBatchResult;

typedef struct {
    uint32_t vertex_capacity;
    uint32_t cursor;
    uint32_t draw_capacity;
    uint32_t draws_used;
} SA3DSGpuBatch;

SA3DSBatchResult sa3ds_gpu_batch_budget(const SA3CCharacter *c,
                                        SA3DSGpuBatch *batch);
void sa3ds_gpu_batch_reset(SA3DSGpuBatch *batch);
SA3DSBatchResult sa3ds_gpu_batch_reserve(SA3DSGpuBatch *batch,
                                         uint32_t vertex_count,
                                         uint32_t *first_vertex);
const char *sa3ds_gpu_batch_result_name(SA3DSBatchResult result);

#endif
