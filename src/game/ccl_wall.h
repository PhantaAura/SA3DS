#ifndef SA3DS_CCL_WALL_H
#define SA3DS_CCL_WALL_H
#include "../sa1/physics/player_state.h"

/* Form 9 response for a sample already transformed into the wall's yaw frame.
 * The caller applies the inverse transform and publishes a task-position
 * correction for both hits; only HARD also rewrites position history. Wall
 * extent Z affects broad phase, while the response plane uses player_radius. */
typedef enum {
    SA3DS_WALL_MISS, SA3DS_WALL_SOFT, SA3DS_WALL_HARD
} SA3DSWallResponse;
SA3DSWallResponse sa3ds_ccl_wall_sample(SA1Vec3 sample, float player_radius,
    float player_half_height, SA1Vec3 extents, float *corrected_z);
/* Sample task displacement in source order. Both task points are in the
 * wall frame. Output is the corrected sample, including its X coordinate. */
SA3DSWallResponse sa3ds_ccl_wall_sweep(SA1Vec3 previous, SA1Vec3 current,
    float player_radius, float player_half_height, SA1Vec3 extents, SA1Vec3 *corrected);
typedef double (*SA3DSWallSine)(void *context,uint16_t angle);
/*41A580: preserve world-space sampling/rounding before yaw conversion.
 *Input previous is source4419C0(player,1), not current tick's incoming task.
 *Both hits change live X/Z; callers write history0 only for HARD. */
SA3DSWallResponse sa3ds_ccl_wall_world_sweep(SA1Vec3 previous,SA1Vec3 current,
    SA1Vec3 wall_center,uint16_t yaw,float radius,float half_height,
    SA1Vec3 dimensions,SA3DSWallSine sine,void *context,SA1Vec3 *corrected);
/*41BAC0 single form9 shape with local center0, as initialized by4D46C0.*/
float sa3ds_ccl_wall_broad_radius(SA1Vec3 dimensions);
#endif
