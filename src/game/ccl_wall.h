#ifndef SA3DS_CCL_WALL_H
#define SA3DS_CCL_WALL_H
#include "../sa1/physics/player_state.h"

/* Form 9 response for a sample already transformed into the wall's yaw frame.
 * The caller applies the inverse transform and publishes a task-position
 * correction only for HARD. */
typedef enum {
    SA3DS_WALL_MISS, SA3DS_WALL_SOFT, SA3DS_WALL_HARD
} SA3DSWallResponse;
SA3DSWallResponse sa3ds_ccl_wall_sample(SA1Vec3 sample, float player_radius,
    float player_half_height, SA1Vec3 extents, float *corrected_z);
#endif
