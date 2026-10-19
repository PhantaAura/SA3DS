#ifndef SA1_PHYSICS_CONSTANTS_H
#define SA1_PHYSICS_CONSTANTS_H

typedef struct {
    float accel_ground, accel_air, brake_accel;
    float drag_ground, drag_air, drag_brake;
    float top_speed, turn_low, turn_high;
    float gravity, jump_speed, jump_cut, max_fall;
    float slope_push, bank_push;
} SA1PhysicsConstants;

/* PROJECT PLACEHOLDERS: not measured Sonic Adventure reference constants. */
extern const SA1PhysicsConstants SA1_PHYSICS_PLACEHOLDER_CONSTANTS;

#endif
