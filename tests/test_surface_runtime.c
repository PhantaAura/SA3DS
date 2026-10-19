#include <assert.h>
#include <stdio.h>

#include "movement.h"
#include "steam_flat.h"
#include "steam_surface.h"
#include "stage.h"

static SA1PlayerState wall_player(float tangent_speed) {
    SA1PlayerState p;
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.pos = (SA1Vec3){0.0f, 18.0f, 238.0f};
    p.vel = (SA1Vec3){0.0f, tangent_speed, 0.0f};
    p.yaw = 0.0f;
    p.grounded = true;
    p.ground_normal = (SA1Vec3){0.0f, 0.0f, -1.0f};
    p.surface_forward = (SA1Vec3){0.0f, 1.0f, 0.0f};
    p.surface_right = (SA1Vec3){1.0f, 0.0f, 0.0f};
    p.surface_id = 100;
    p.action_state = SA1_ACTION_ROLL;
    p.ball_form = true;
    return p;
}

int main(void) {
    SA1Input input = {0};
    const SA1PhysicsWorld *world = sa3ds_movement_lab_world();
    SA1SteamSurfaceDebug dbg;

    /* A fresh reset has no established surface id, so one initialization
       acquisition is allowed. Collision/support is still queried after motion. */
    SA1PlayerState fresh;
    sa1_player_reset(&fresh, world);
    sa1_physics_step_steam_sonic_surface(&fresh, &input, 0.0f, world,
                                         &SA1_PHYSICS_PLACEHOLDER_CONSTANTS,
                                         &dbg);
    assert(dbg.initial_support_queries == 1u);
    assert(dbg.post_motion_support_queries == 1u);
    assert(dbg.support_queries == 2u);
    assert(fresh.surface_id >= 0);
    assert(dbg.pc2004_collision_work_reconstructed == 1);
    assert(dbg.pc2004_retail_polygon_solver_pending == 1);
    assert(dbg.pc2004_collision_radius == 5.0f);
    assert(dbg.pc2004_collision_find_count == 1u);

    /* Once support is established, movement uses the previous frame's support
       first. There is no speculative pre-movement refresh; only the
       PSetPosition/PResetPosition-like post-motion support pass remains. */
    sa1_physics_step_steam_sonic_surface(&fresh, &input, 0.0f, world,
                                         &SA1_PHYSICS_PLACEHOLDER_CONSTANTS,
                                         &dbg);
    assert(dbg.initial_support_queries == 0u);
    assert(dbg.post_motion_support_queries == 1u);
    assert(dbg.support_queries == 1u);

    /* 0.0.8.8's forced 0.3/1.16 wall fall was not binary-backed for the exact
       2004 PSetPosition/PResetPosition bodies. It must no longer force a
       detach in the primary PC profile. Geometric support loss remains active. */
    SA1PlayerState slow = wall_player(0.90f);
    sa1_physics_step_steam_sonic_surface(&slow, &input, 0.0f, world,
                                         &SA1_PHYSICS_PLACEHOLDER_CONSTANTS,
                                         &dbg);
    assert(slow.grounded);
    assert(slow.surface_id == 100);
    assert(dbg.legacy_low_speed_fall_disabled == 1);
    assert(dbg.initial_support_queries == 0u);
    assert(dbg.post_motion_support_queries == 1u);

    SA1PlayerState fast = wall_player(2.00f);
    sa1_physics_step_steam_sonic_surface(&fast, &input, 0.0f, world,
                                         &SA1_PHYSICS_PLACEHOLDER_CONSTANTS,
                                         &dbg);
    assert(fast.grounded);
    assert(fast.surface_id == 100);

    printf("surface runtime support-order tests: PASS\n");
    return 0;
}
