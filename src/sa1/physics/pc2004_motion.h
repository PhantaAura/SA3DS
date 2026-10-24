#ifndef SA1_PHYSICS_PC2004_MOTION_H
#define SA1_PHYSICS_PC2004_MOTION_H

#include "steam_player_parameters.h"

typedef struct {
    float forward;
    float normal;
    float side;
} SA1PC2004LocalVector;

typedef struct {
    /* taskwk status low bits 0/1 are the grounded/support flags tested by
     * PGetBreak/PGetInertia. */
    int grounded_status;
    /* taskwk status 0x100 branch used by PGetInertia. */
    int status_0x100;
    /* taskwk status 0x4000 bypasses both helpers. */
    int status_0x4000;
    /* playerwk Up value. On ordinary supported geometry this is the world-Y
     * component of the support-up vector after PResetPosition. */
    float up_y;
    /* motionwk2 +0x3C. PInitialize writes 1.0f for the normal player path. */
    float acceleration_multiplier;
} SA1PC2004MotionContext;

typedef struct {
    int used_zero_speed_lim_frict;
    int used_ground_friction;
    int clamped_forward_zero_cross;
    int clamped_side_zero_cross;
    int used_inertia_negative_up_flip;
    int used_inertia_status_0x100_drag;
} SA1PC2004MotionDebug;

/* Portable arithmetic specialization of 2004-PC PGetBreak (0x448E50) for the
 * normal player path. `local_force` is the already player-local value of
 * Gravity*weight + motionwk2::SomeCollisionVector. The helper reproduces the
 * retail drag, run_break, grd_frict/grd_frict_z and zero-cross guards. It does
 * not emulate the final angle helper call because that does not change the
 * AddVelocity vector returned by this specialization. */
SA1PC2004LocalVector sa1_pc2004_pgetbreak_add_velocity(
    SA1PC2004LocalVector speed,
    SA1PC2004LocalVector local_force,
    const SA1PC2004MotionContext *context,
    const SA1SteamPlayerParameters *p,
    SA1PC2004MotionDebug *debug_out);

/* Portable arithmetic specialization of 2004-PC PGetInertia (0x443650).
 * `local_force` has the same meaning as above. The opaque pending-collision
 * impulse buffer consumed through playerwk+0x64 is intentionally excluded;
 * the Movement Lab has no dynamic collision object producing that buffer.
 * All ordinary drag/Up/status branches are represented. */
SA1PC2004LocalVector sa1_pc2004_pgetinertia_add_velocity(
    SA1PC2004LocalVector speed,
    SA1PC2004LocalVector local_force,
    const SA1PC2004MotionContext *context,
    const SA1SteamPlayerParameters *p,
    SA1PC2004MotionDebug *debug_out);

/* PGetSpeed prevents forward/side AddVelocity steps from crossing through
 * zero; local normal is integrated directly at 0x444031. This helper
 * implements the tangential invariant without promoting the
 * remaining low-speed/status checks to a claim about every action. */
float sa1_pc2004_integrate_axis_no_cross(float speed, float add_velocity,
                                         int *clamped_out);

#endif
