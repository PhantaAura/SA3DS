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

    /* PC2004 EntityData2+0x38 scratch input used only to pace animation.
     * It is cleared before each movement tick and receives the forward force
     * left over by specific ground-friction clamps. It must never feed back
     * into SA3DS movement. */
    float animation_accel_residual;
    bool animation_accel_residual_valid;

    /* Presentation-only bridge for ordinary static-contact landing/idle
     * selection. Never feeds movement, forces, or collision arbitration. */
    bool animation_input_active;
    bool animation_input_valid;
    float animation_landing_world_y;
    uint8_t animation_landing_air_action; /* supported retail equivalents 8/12 */
    bool animation_landing_valid;
} SA1PlayerState;

#endif
