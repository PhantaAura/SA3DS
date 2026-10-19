#ifndef SA1_PHYSICS_PLAYER_STATE_H
#define SA1_PHYSICS_PLAYER_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct { float x, y, z; } SA1Vec3;

typedef enum {
    SA1_ACTION_GROUNDED = 0,
    SA1_ACTION_AIRBORNE = 1,
    SA1_ACTION_SPINDASH_CHARGE = 2,
    SA1_ACTION_ROLL = 3
} SA1ActionState;

typedef struct {
    SA1Vec3 pos;
    SA1Vec3 vel;
    float yaw;
    bool grounded;
    float peak_speed;
    float peak_y;
    uint64_t sim_frame;
    SA1Vec3 ground_normal;
    float ground_slope_radians;
    int32_t surface_id;
    SA1ActionState action_state;
    uint32_t jump_ticks;
    bool ball_form;
    float spindash_charge;
    uint32_t spindash_ticks;

    /* 0.0.8.7 general-surface frame. ground_normal remains the externally
     * visible support normal; forward/right are tangent to that support. */
    SA1Vec3 surface_forward;
    SA1Vec3 surface_right;
    float support_distance;
    uint32_t surface_flags;
} SA1PlayerState;

#endif
