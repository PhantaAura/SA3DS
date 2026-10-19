#ifndef SA3DS_GAME_SONIC_POSE_METRICS_H
#define SA3DS_GAME_SONIC_POSE_METRICS_H
#include <stddef.h>
#include <stdint.h>
#include "sa3c_character.h"
#include "sa3c_pose.h"

typedef struct {
    float min[3], max[3];
    float root[3];
    uint32_t visible_vertices, triangles, draw_calls;
    uint16_t root_node;
} SA3DSPoseMetrics;

/* Source vertex coordinates transformed by the selected Ninja hierarchy.
   No texture, GPU, physics, or 3DS-specific dependencies. Returns 0 on
   malformed geometry, empty draw, nonfinite coordinates or failed pose. */
int sa3ds_pose_metrics(const SA3CCharacter *character, uint16_t root_node,
    const SA3CMatrix *world, size_t world_count, SA3DSPoseMetrics *out);
#endif
