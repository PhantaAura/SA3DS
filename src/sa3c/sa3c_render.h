#ifndef SA3DS_SA3C_RENDER_H
#define SA3DS_SA3C_RENDER_H

#include <stddef.h>
#include <stdint.h>
#include "sa3c_pose.h"

typedef struct {
    uint16_t node_index;
    uint16_t model_index;
    uint16_t mesh_index;
    uint16_t material_index;
    uint16_t texture_slot;
    uint16_t source_primitive;
    uint32_t first_index;
    uint32_t index_count;
    SA3CMatrix world;
} SA3CDrawCommand;

typedef struct {
    uint32_t visited_nodes;
    uint32_t visible_model_nodes;
    uint32_t draw_calls;
    uint32_t indices;
    uint32_t triangles;
} SA3CRenderStats;

typedef int (*SA3CDrawVisitor)(const SA3CDrawCommand *command, void *user);

/* Basic SADX PC UV normalization confirmed by SA Tools UV.cs. */
static inline float sa3c_basic_uv_normalize(float raw) { return raw / 255.0f; }

/* Emit allocation-free draw commands for one already-built world pose. */
SA3CPoseResult sa3c_render_emit(
    const SA3CCharacter *c,
    uint16_t root_node,
    const SA3CMatrix *world,
    size_t world_count,
    SA3CDrawVisitor visitor,
    void *user,
    SA3CRenderStats *stats);

#endif
