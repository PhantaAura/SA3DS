#ifndef SA1_PHYSICS_STEAM_FLAT_H
#define SA1_PHYSICS_STEAM_FLAT_H

#include "movement.h"
#include "steam_input.h"
#include "steam_player_parameters.h"

/* Which Steam turning helper was selected by the flat-ground reconstruction. */
typedef enum {
    SA1_STEAM_TURN_NONE = 0,
    SA1_STEAM_TURN_GENERAL = 1,
    SA1_STEAM_TURN_QUICK = 2,
    SA1_STEAM_TURN_SLOW = 3
} SA1SteamTurnKind;

/*
 * Diagnostics for parity work. Values are player-local: forward is X in the
 * original SADX player work, side is Z. These fields are not game state.
 */
typedef struct {
    float forward_speed_before;
    float side_speed_before;
    float forward_speed_after;
    float side_speed_after;
    float accel_forward;
    float accel_side;
    float requested_force;
    float resistance_force;
    float filtered_input_magnitude;
    float post_deadzone_x;
    float post_deadzone_z;
    unsigned filtered_input_angle_bams;
    unsigned angle_delta_bams;
    unsigned turn_step_bams;
    SA1SteamTurnKind turn_kind;
    int had_analog;
    int used_brake_branch;
    int crossed_zero;
    int side_stopped_at_jog_threshold;
} SA1SteamFlatDebug;

/*
 * 0.0.8.8 Ground + Air + Ball + Surface Evidence Candidate.
 *
 * On a flat, grounded surface this reconstructs the verified Steam SADX path:
 * controller shaping -> PGetAcceleration branch/drag logic -> turning helper
 * behavior -> local acceleration -> PGetSpeed-style integration.
 *
 * Air/jump routes through the 0.0.8.5 Steam air candidate and flat ball states
 * route through the 0.0.8.7 general-surface candidate. Grounded
 * non-flat collision now routes through the 0.0.8.7 arbitrary-contact
 * surface frame while this flat specialization remains unchanged. Reference traces from
 * the original executable are still required before calling this proven 1:1
 * behavior.
 */
void sa1_physics_step_steam_sonic_flat(
    SA1PlayerState *player,
    const SA1Input *input,
    float camera_yaw,
    const SA1PhysicsWorld *world,
    const SA1PhysicsConstants *compat_constants,
    SA1SteamFlatDebug *debug_out);

#endif
