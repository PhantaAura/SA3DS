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
    /* Exact PC2004 EntityData1.Action when the current reconstructed path
     * establishes it. The broad SA1ActionState remains the control state. */
    uint8_t pc2004_action_state;
    bool pc2004_action_state_valid;
    uint32_t jump_ticks;
    int16_t pc2004_control_timer; /* GetAnalog gate; tail decrements after movement. */
    int16_t pc2004_air_attack_timer,pc2004_air_attack_age;
    bool ball_form;
    float spindash_charge;
    uint32_t spindash_ticks;

    /* 0.0.8.7 general-surface frame. ground_normal remains the externally
     * visible support normal; forward/right are tangent to that support. */
    SA1Vec3 surface_forward;
    SA1Vec3 surface_right;
    /* The general task frame remains authoritative through airborne ticks.
     * Cleared by reset or an explicitly selected flat grounded tick. */
    bool surface_frame_active;
    float support_distance;
    uint32_t surface_flags;

    /* Completed general-surface tick's signed player-local Speed.x.
     * The flat specialization leaves this invalid and uses its yaw projection.
     * Presentation only: never feeds forces, steering, or collision. */
    float animation_forward_speed;
    bool animation_forward_speed_valid;

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

    /* Action 8 can be reached by a jump (request 14) or by rolling off
     * support (retain request 15). This is presentation-only provenance. */
    uint16_t animation_airborne_request;
    bool animation_airborne_request_valid;
} SA1PlayerState;

#endif
