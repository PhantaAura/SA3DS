#ifndef SA3DS_PHYSICS_COMPAT_H
#define SA3DS_PHYSICS_COMPAT_H

/* Compatibility surface for the 0.0.7 Movement Lab renderer. */
#include "../../src/sa1/physics/movement.h"
#include "../../src/sa1/physics/steam_flat.h"
#include "../../src/game/stage.h"

typedef SA1Vec3 SaVec3;
typedef SA1PlayerState SaPlayer;
typedef SA1Surface SaSurface;
typedef SA1Input SaInput;

static inline void sa_physics_reset(SaPlayer *player) {
    sa1_player_reset(player, sa3ds_movement_lab_world());
}
static inline SaSurface sa_course_surface(float x, float z) {
    return sa3ds_movement_lab_surface(0, x, z);
}
static inline void sa_physics_step(SaPlayer *player, const SaInput *input, float camera_yaw) {
    sa1_physics_step_steam_sonic_flat(player, input, camera_yaw,
                     sa3ds_movement_lab_world(),
                     &SA1_PHYSICS_PLACEHOLDER_CONSTANTS, NULL);
}
static inline float sa_horizontal_speed(const SaPlayer *player) {
    return sa1_horizontal_speed(player);
}

#endif
