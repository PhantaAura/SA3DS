#include "sa3c_gpu_batch.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#define SA3DS_INVALID_U16 0xFFFFu
/* Keep the total linear-memory reservation bounded on Old 3DS. At 20 bytes
 * per vertex this is <= 8 MiB, still subject to linearAlloc succeeding. */
#define SA3DS_BATCH_MAX_VERTICES (8u * 1024u * 1024u / 20u)

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

SA3DSBatchResult sa3ds_gpu_batch_budget(const SA3CCharacter *c,
                                        SA3DSGpuBatch *batch) {
    if (!c || !c->data || !batch) return SA3DS_BATCH_ARGUMENT;
    memset(batch, 0, sizeof(*batch));
    const SA3CSection *nodes = &c->sections[0];
    const SA3CSection *models = &c->sections[1];
    const SA3CSection *meshes = &c->sections[2];
    if (nodes->stride < 2 || models->stride < 8 || meshes->stride < 12)
        return SA3DS_BATCH_ARGUMENT;
    if ((size_t)nodes->offset + nodes->size > c->size ||
        (size_t)models->offset + models->size > c->size ||
        (size_t)meshes->offset + meshes->size > c->size)
        return SA3DS_BATCH_ARGUMENT;

    uint32_t total = 0, draws = 0;
    for (uint32_t ni = 0; ni < nodes->count; ++ni) {
        const uint8_t *n = c->data + nodes->offset + (size_t)ni * nodes->stride;
        uint16_t model_index = rd16(n);
        if (model_index == SA3DS_INVALID_U16) continue;
        if ((uint32_t)model_index >= models->count) return SA3DS_BATCH_BAD_REFERENCE;
        const uint8_t *m = c->data + models->offset +
                           (size_t)model_index * models->stride;
        uint32_t first = rd32(m), count = rd32(m + 4);
        if (first > meshes->count || count > meshes->count - first)
            return SA3DS_BATCH_BAD_REFERENCE;
        for (uint32_t j = 0; j < count; ++j) {
            const uint8_t *mesh = c->data + meshes->offset +
                                  (size_t)(first + j) * meshes->stride;
            uint32_t vertices = rd32(mesh + 8);
            if (vertices == 0) continue;
            if (vertices > SA3DS_BATCH_MAX_VERTICES - total || draws == UINT32_MAX)
                return SA3DS_BATCH_OVERFLOW;
            total += vertices;
            ++draws;
        }
    }
    if (total == 0) return SA3DS_BATCH_EMPTY;
    batch->vertex_capacity = total;
    batch->draw_capacity = draws;
    return SA3DS_BATCH_OK;
}

void sa3ds_gpu_batch_reset(SA3DSGpuBatch *batch) {
    if (!batch) return;
    batch->cursor = 0;
    batch->draws_used = 0;
}

SA3DSBatchResult sa3ds_gpu_batch_reserve(SA3DSGpuBatch *batch,
                                         uint32_t vertex_count,
                                         uint32_t *first_vertex) {
    if (!batch || !first_vertex || !vertex_count) return SA3DS_BATCH_ARGUMENT;
    if (batch->cursor > batch->vertex_capacity ||
        vertex_count > batch->vertex_capacity - batch->cursor ||
        batch->draws_used >= batch->draw_capacity)
        return SA3DS_BATCH_CAPACITY;
    *first_vertex = batch->cursor;
    batch->cursor += vertex_count;
    ++batch->draws_used;
    return SA3DS_BATCH_OK;
}

const char *sa3ds_gpu_batch_result_name(SA3DSBatchResult result) {
    switch (result) {
        case SA3DS_BATCH_OK: return "OK";
        case SA3DS_BATCH_ARGUMENT: return "INVALID_ARGUMENT";
        case SA3DS_BATCH_BAD_REFERENCE: return "BAD_REFERENCE";
        case SA3DS_BATCH_OVERFLOW: return "LINEAR_MEMORY_BUDGET_EXCEEDED";
        case SA3DS_BATCH_EMPTY: return "NO_VISIBLE_GEOMETRY";
        case SA3DS_BATCH_CAPACITY: return "FRAME_VERTEX_ARENA_EXHAUSTED";
        default: return "UNKNOWN";
    }
}
