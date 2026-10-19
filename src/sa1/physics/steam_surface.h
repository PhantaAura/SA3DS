#ifndef SA1_PHYSICS_STEAM_SURFACE_H
#define SA1_PHYSICS_STEAM_SURFACE_H

#include "movement.h"
#include "steam_input.h"
#include "steam_player_parameters.h"

typedef struct {
    float forward_speed_before;
    float forward_speed_after;
    float side_speed_after;
    float tangent_gravity_forward;
    float tangent_gravity_side;
    float support_distance;
    float normal_alignment;
    unsigned turn_step_bams;
    int used_general_contact;
    int snapped_to_support;
    int detached;
    int ceiling_detach_candidate;
    int legacy_low_speed_fall_disabled;
    unsigned support_queries;
    unsigned initial_support_queries;
    unsigned post_motion_support_queries;
    float surface_force_forward_scale;
    float surface_force_side_scale;
    float normal_unstick_bias;
    float pc2004_local_normal_force;
    float pc2004_add_forward;
    float pc2004_add_side;
    int used_pc2004_pgetbreak;
    int used_pc2004_pgetinertia;
    int pc2004_pending_collision_impulse_omitted;
    float pc2004_collision_radius;
    unsigned pc2004_collision_find_count;
    int pc2004_collision_work_reconstructed;
    int pc2004_retail_polygon_solver_pending;
} SA1SteamSurfaceDebug;

/*
 * 0.0.8.8.9 general-surface parity candidate.
 *
 * The supported 2004 PC action handlers run movement first and then call
 * PSetPosition -> PResetPosition. 0.0.8.8.7 therefore no longer performs a
 * speculative support refresh before every grounded movement step. The old
 * 0.3/1.16 forced-detach helper is also disabled for the 2004 profile because
 * the exact 2004 PSetPosition/PResetPosition bodies do not directly reference
 * the 1.16 constant. 0.0.8.8.8 additionally routes ball-state tangent motion through exact-2004 PGetBreak/PGetInertia arithmetic specializations. 0.0.8.8.9 reconstructs the exact 0x84-byte 2004 collision-work layout and initialization while keeping the synthetic lab query stand-off separate from retail polygon-solver semantics.
 */
void sa1_physics_step_steam_sonic_surface(
    SA1PlayerState *player,
    const SA1Input *input,
    float camera_yaw,
    const SA1PhysicsWorld *world,
    const SA1PhysicsConstants *compat_constants,
    SA1SteamSurfaceDebug *debug_out);

#endif
