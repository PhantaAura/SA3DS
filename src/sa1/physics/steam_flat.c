#include "pc2004_motion.h"
#include "pc2004_ground_accel.h"
#include "steam_flat.h"
#include "steam_air.h"
#include "steam_ball.h"
#include "steam_surface.h"
#include <math.h>
#include <string.h>

#define SA1_PI 3.14159265358979323846f
#define SA1_TAU (2.0f * SA1_PI)
#define BAMS_FULL 65536.0f
#define BAMS_22_5 0x1000u
#define BAMS_45   0x2000u

/* Verified constants used by the supported Steam executable. */
#define SADX_UPRIGHT_TURN_BLEND 0.99f

typedef struct {
    float forward;
    float vertical;
    float side;
} SA1LocalSpeed;

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float wrap_angle(float a) {
    while (a > SA1_PI) a -= SA1_TAU;
    while (a < -SA1_PI) a += SA1_TAU;
    return a;
}

static unsigned rad_to_bams_abs(float radians) {
    float turns = fabsf(wrap_angle(radians)) / SA1_TAU;
    unsigned v = (unsigned)(turns * BAMS_FULL + 0.5f);
    if (v > 0x8000u) v = 0x8000u;
    return v;
}

static float bams_to_rad(unsigned bams) {
    return ((float)bams / BAMS_FULL) * SA1_TAU;
}

static float approach_angle_bams(float angle, float target, unsigned step_bams) {
    float diff = sa1_pc2004_turn_delta(target - angle);
    float step = bams_to_rad(step_bams);
    if (diff > step) diff = step;
    if (diff < -step) diff = -step;
    return wrap_angle(angle + diff);
}

static SA1LocalSpeed world_to_local(float yaw, SA1Vec3 velocity) {
    float sy = sinf(yaw), cy = cosf(yaw);
    SA1LocalSpeed out;
    out.forward = velocity.x * sy + velocity.z * cy;
    out.vertical = velocity.y;
    out.side = velocity.x * cy - velocity.z * sy;
    return out;
}

static SA1Vec3 local_to_world(float yaw, SA1LocalSpeed speed) {
    float sy = sinf(yaw), cy = cosf(yaw);
    SA1Vec3 out;
    out.x = sy * speed.forward + cy * speed.side;
    out.y = speed.vertical;
    out.z = cy * speed.forward - sy * speed.side;
    return out;
}

/* Steam helper 0x004E2480. */
static unsigned steam_general_turn_step(unsigned delta_bams) {
    if (delta_bams > BAMS_45) return 0x0800u;
    if (delta_bams > BAMS_22_5) return delta_bams >> 2;
    return delta_bams >> 3;
}

/* Steam helper 0x004E2770. */
static unsigned steam_quick_turn_step(void) {
    return BAMS_45;
}

/* PC2004 0x443E60 uses truncation after the stored sqrt argument. */
static unsigned steam_slow_turn_step(float speed,
                                     const SA1SteamPlayerParameters *p) {
    return sa1_pc2004_slow_turn_step(speed, p->dash_speed);
}

/* The PC2004 turn helpers rotate the player frame and then transform/blend the
 * local speed vector. Keeping world velocity as SA3DS's authoritative state
 * lets us reproduce that transformation explicitly. */
static SA1LocalSpeed apply_turn_helper(SA1PlayerState *player,
                                       SA1LocalSpeed old_local,
                                       float target,
                                       SA1SteamTurnKind kind,
                                       unsigned step_bams) {
    SA1Vec3 old_world = local_to_world(player->yaw, old_local);
    player->yaw = approach_angle_bams(player->yaw, target, step_bams);
    SA1LocalSpeed transformed = world_to_local(player->yaw, old_world);

    if (kind == SA1_STEAM_TURN_QUICK)
        return transformed;

    if (kind == SA1_STEAM_TURN_GENERAL || kind == SA1_STEAM_TURN_SLOW) {
        /* PC2004 0x443C50/0x443E60 on upright ground: retain 99% of
         * the pre-turn local velocity and mix in 1% of the vector transformed
         * into the new player frame. The previous implementation had these
         * operands reversed. */
        transformed.forward = old_local.forward * SADX_UPRIGHT_TURN_BLEND +
                              transformed.forward * (1.0f - SADX_UPRIGHT_TURN_BLEND);
        transformed.vertical = old_local.vertical * SADX_UPRIGHT_TURN_BLEND +
                               transformed.vertical * (1.0f - SADX_UPRIGHT_TURN_BLEND);
        transformed.side = old_local.side * SADX_UPRIGHT_TURN_BLEND +
                           transformed.side * (1.0f - SADX_UPRIGHT_TURN_BLEND);
    }
    return transformed;
}

/* Exact branch structure from the flat/upright portion of Steam
 * PGetAcceleration. The input magnitude is already SADX-filtered. */
static float select_analog_force(float speed, float magnitude,
                                 const SA1SteamPlayerParameters *p) {
    if (speed < p->max_x_spd) {
        if (speed < p->jog_speed) {
            if (magnitude > 0.5f)
                return magnitude * p->run_accel;
            if (speed >= p->jog_speed * 0.4f)
                return 0.0f;
            return magnitude * p->run_accel;
        }

        if (speed < p->run_speed) {
            if (magnitude > 0.7f)
                return magnitude * p->run_accel;
            /* The executable computes (run_speed + run_speed) * 0.5 here;
             * while speed is still below run_speed this falls through to the
             * normal run_accel contribution. */
            return magnitude * p->run_accel;
        }

        if (speed < p->rush_speed) {
            float force = magnitude * p->run_accel;
            if (magnitude <= 0.9f) force *= 0.3f;
            return force;
        }
        return magnitude * p->run_accel;
    }

    return magnitude * p->run_accel * 0.4f;
}

static void steam_flat_horizontal(SA1PlayerState *player, const SA1Input *input,
                                  float camera_yaw, SA1SteamFlatDebug *dbg) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1SADXFilteredAnalog analog =
        sa1_sadx_filter_logical_analog(input->move_x, input->move_z);
    SA1LocalSpeed local = world_to_local(player->yaw, player->vel);
    SA1LocalSpeed original_local = local;
    float speed_for_branches = local.forward;
    float resistance = 0.0f;
    float force = 0.0f;
    unsigned delta_bams = 0;
    unsigned turn_step = 0;
    SA1SteamTurnKind turn_kind = SA1_STEAM_TURN_NONE;
    int brake_branch = 0;
    int crossed_zero = 0;
    int side_stopped = 0;

    /* Verified flat/upright resistance branches in Steam PGetAcceleration. */
    if (analog.active) {
        if (speed_for_branches > p->max_x_spd) {
            resistance = (speed_for_branches - p->max_x_spd) *
                         p->air_resist * 1.7f;
        } else if (speed_for_branches < 0.0f) {
            resistance = p->air_resist * speed_for_branches;
        }
    } else if (speed_for_branches > p->run_speed || speed_for_branches < 0.0f) {
        resistance = p->air_resist * speed_for_branches;
    }

    if (analog.active) {
        float desired = wrap_angle(camera_yaw + analog.angle_radians);
        delta_bams = rad_to_bams_abs(desired - player->yaw);
        force = select_analog_force(speed_for_branches, analog.magnitude, p);

        if (speed_for_branches == 0.0f && delta_bams > BAMS_22_5) {
            force = 0.0f;
            turn_kind = SA1_STEAM_TURN_QUICK;
            turn_step = steam_quick_turn_step();
            local = apply_turn_helper(player, local, desired, turn_kind, turn_step);
        } else {
            float low_turn_boundary = (p->run_speed + p->jog_speed) * 0.5f;
            if (low_turn_boundary > speed_for_branches || delta_bams <= BAMS_22_5) {
                if (speed_for_branches >= p->jog_speed && delta_bams < BAMS_22_5) {
                    turn_kind = SA1_STEAM_TURN_SLOW;
                    turn_step = steam_slow_turn_step(speed_for_branches, p);
                } else {
                    if (speed_for_branches >= p->jog_speed &&
                        speed_for_branches <= p->rush_speed &&
                        delta_bams > BAMS_45)
                        force *= 0.8f;
                    turn_kind = SA1_STEAM_TURN_GENERAL;
                    turn_step = steam_general_turn_step(delta_bams);
                }
            } else {
                /* Opposing/high-angle input at speed requests slow_down instead
                 * of reverse thrust. */
                force = p->slow_down;
                brake_branch = 1;
                turn_kind = SA1_STEAM_TURN_GENERAL;
                turn_step = steam_general_turn_step(delta_bams);
            }
            local = apply_turn_helper(player, local, desired, turn_kind, turn_step);
        }
    } else {
        if (speed_for_branches > 0.0f) force = p->slow_down;
        else if (speed_for_branches < 0.0f) force = -p->slow_down;
    }

    /* Ordinary upright static terrain: the source external-force vector is
     * zero after the completed update tail. Contact does not cancel gravity
     * before the acceleration clamp. Capture resistance in the old frame. */
    SA1PC2004GroundAccelInput clamp_input = {
        .force_after_resistance = {resistance,
            -p->weight + original_local.vertical * p->air_resist_y,
            original_local.side * p->air_resist_z},
        .input_force = force,
        .speed_after_turn = {local.forward, local.vertical, local.side},
        .up = 1.0f, .friction_multiplier = 1.0f,
        .analog_active = analog.active
    };
    SA1PC2004GroundAccelResult acceleration;
    SA1PC2004LocalVector integrated;
    sa1_pc2004_ground_accel_clamp(&clamp_input, p, &acceleration);
    sa1_pc2004_pgetspeed_local(clamp_input.speed_after_turn,
        acceleration.add_velocity, analog.active, 1.0f, 0, p, &integrated);
    float accel_forward = acceleration.add_velocity.forward;
    float accel_side = acceleration.add_velocity.side;
    float raw_forward = local.forward + accel_forward;
    crossed_zero = (local.forward > 0.0f && raw_forward < 0.0f) ||
                   (local.forward < 0.0f && raw_forward > 0.0f);
    side_stopped = local.side != 0.0f && integrated.side == 0.0f;
    local.forward = clampf(integrated.forward, -p->lim_h_spd, p->lim_h_spd);
    local.side = clampf(integrated.side, -p->lim_h_spd, p->lim_h_spd);
    local.vertical = clampf(integrated.normal, -p->lim_v_spd, p->lim_v_spd);
    player->vel = local_to_world(player->yaw, local);
    player->animation_accel_residual = acceleration.animation_residual;
    player->animation_accel_residual_valid = true;

    if (dbg) {
        memset(dbg, 0, sizeof(*dbg));
        dbg->forward_speed_before = original_local.forward;
        dbg->side_speed_before = original_local.side;
        dbg->forward_speed_after = local.forward;
        dbg->side_speed_after = local.side;
        dbg->accel_forward = accel_forward;
        dbg->accel_side = accel_side;
        dbg->requested_force = force;
        dbg->resistance_force = resistance;
        dbg->filtered_input_magnitude = analog.magnitude;
        dbg->post_deadzone_x = analog.post_deadzone_x;
        dbg->post_deadzone_z = analog.post_deadzone_z;
        dbg->filtered_input_angle_bams = analog.angle_bams;
        dbg->angle_delta_bams = delta_bams;
        dbg->turn_step_bams = turn_step;
        dbg->turn_kind = turn_kind;
        dbg->had_analog = analog.active;
        dbg->used_brake_branch = brake_branch;
        dbg->crossed_zero = crossed_zero;
        dbg->side_stopped_at_jog_threshold = side_stopped;
    }
}

void sa1_physics_step_steam_sonic_flat(
    SA1PlayerState *player,
    const SA1Input *input,
    float camera_yaw,
    const SA1PhysicsWorld *world,
    const SA1PhysicsConstants *compat_constants,
    SA1SteamFlatDebug *debug_out) {

    SA1Surface surface = world->query_surface ?
        world->query_surface(world->context, player->pos.x, player->pos.z) :
        (SA1Surface){0};
    int just_jumped = 0;
    player->animation_input_active = sa1_sadx_filter_logical_analog(
        input->move_x, input->move_z).active != 0;
    player->animation_input_valid = true;
    player->animation_landing_valid = false;
    player->animation_forward_speed_valid = false;

    /* Mirrors the per-tick clear at PC2004 0x00446F10. A path marks the value
     * valid only where its residual behavior has been recovered. */
    player->animation_accel_residual = 0.0f;
    player->animation_accel_residual_valid = false;

    int attack_dispatch=!player->grounded&&player->pc2004_action_state_valid&&
        (player->pc2004_action_state==14||(player->pc2004_action_state==8&&
        (input->jump_pressed||input->action_pressed)));
    if(attack_dispatch&&!player->surface_frame_active){
        /* Flat steering owns yaw; reset's old cached axes are not a frame. */
        SA1SurfaceFrame attack_frame=sa1_surface_frame_make(player->ground_normal,
            (SA1Vec3){sinf(player->yaw),0,cosf(player->yaw)});
        player->surface_forward=attack_frame.forward;player->surface_right=attack_frame.right;
    }

    /* A slope launch/departure does not become world-upright on its second
     * airborne tick. Continue the retained task-frame path until grounded. */
    if (attack_dispatch || (!player->grounded && player->surface_frame_active) ||
        (player->pc2004_action_state_valid &&
         (player->pc2004_action_state == 9 || player->pc2004_action_state == 10))) {
        if (debug_out) memset(debug_out, 0, sizeof(*debug_out));
        sa1_physics_step_steam_sonic_surface(player, input, camera_yaw,
                                              world, compat_constants, NULL);
        return;
    }

    /* 0.0.8.7 preserves the verified flat/upright specialization exactly and
     * routes non-flat/arbitrary support through the new general contact frame. */
    if (player->grounded) {
        SA1Vec3 probe = {-player->ground_normal.x,
                         -player->ground_normal.y,
                         -player->ground_normal.z};
        SA1Contact support = sa1_world_query_contact(world, player->pos,
                                                      probe, 2.35f);
        int arbitrary_support = support.valid &&
            (support.normal.y < 0.99995f || support.surface_flags != 0);
        if (!surface.valid || surface.slope_radians > 0.0025f || arbitrary_support) {
            if (debug_out) memset(debug_out, 0, sizeof(*debug_out));
            sa1_physics_step_steam_sonic_surface(player, input, camera_yaw,
                                                  world, compat_constants, NULL);
            return;
        }

        player->surface_frame_active = false;

        /* PC2004 Sonic_Act1 tests jump before charge entry/roll termination
         * and before the resulting action's movement dispatch. Action 4
         * deliberately has no jump test; action 5 does (0x497446). */
        if (player->action_state != SA1_ACTION_SPINDASH_CHARGE && input->jump_pressed) {
            sa1_steam_sonic_begin_jump(player);
            just_jumped = 1;
            if (debug_out) memset(debug_out, 0, sizeof(*debug_out));
        } else {
            int ball_handled = sa1_steam_sonic_ball_ground_step(
                player, input, camera_yaw, NULL);
            if (!ball_handled) {
                /* Sonic_Act1 action 2 calls 0x494FF0 before its movement
                 * update.  Preserve that entry-state deciding condition;
                 * using the post-acceleration speed can miss the exact
                 * action-2 -> action-1 stop transition. */
                float selector_forward = player->vel.x * sinf(player->yaw) +
                                         player->vel.z * cosf(player->yaw);
                int selector_input_active = player->animation_input_active;
                steam_flat_horizontal(player, input, camera_yaw, debug_out);
                if (player->pc2004_action_state_valid) {
                    if (player->pc2004_action_state == 2 &&
                        !selector_input_active && selector_forward == 0.0f) {
                        player->pc2004_action_state = 1;
                    } else if (player->pc2004_action_state == 1 &&
                               selector_input_active) {
                        /* Action 1 reaches the shared 0x49A83B write when
                         * 0x43BDC0 reports processed analog: action 2 and
                         * animation request 9. Residual speed alone does not
                         * take that branch. */
                        player->pc2004_action_state = 2;
                    }
                }
            } else if (debug_out) {
                /* Ball diagnostics live in SA1SteamBallDebug; do not expose
                 * stale flat-ground diagnostics during a ball-state tick. */
                memset(debug_out, 0, sizeof(*debug_out));
            }
        }
    } else if (debug_out) {
        memset(debug_out, 0, sizeof(*debug_out));
    }

    if (!player->grounded) {
        /* In Sonic's state logic the held-jump helper is an airborne-state
         * operation. A freshly initiated jump therefore starts boosting on
         * the following simulation tick, then PGetAccelerationAir runs. */
        if (!just_jumped && (!player->pc2004_action_state_valid ||
                             player->pc2004_action_state == 8))
            (void)sa1_steam_sonic_apply_jump_hold(player, input->jump_held);
        sa1_steam_sonic_air_step(player, input, camera_yaw, NULL);
        /* PGetAccelerationAir does not populate the ground-friction scratch. */
        player->animation_accel_residual_valid = true;
    }

    player->pos.x += player->vel.x;
    player->pos.y += player->vel.y;
    player->pos.z += player->vel.z;

    surface = world->query_surface(world->context, player->pos.x, player->pos.z);
    if (surface.valid && player->vel.y <= 0.0f &&
        player->pos.y >= surface.ground_y - 0.35f &&
        player->pos.y <= surface.ground_y + 1.6f) {
        int landed_from_air = !player->grounded &&
                              player->action_state == SA1_ACTION_AIRBORNE;
        if (landed_from_air) {
            /* Retail motionwk+8 is world Y, separate from the local normal
             * zeroed by PResetPosition. Preserve the corresponding velocity
             * before this static-floor adapter destroys it. */
            player->animation_landing_world_y = player->vel.y;
            player->animation_landing_air_action =
                (player->pc2004_action_state_valid &&
                 (player->pc2004_action_state == 8 ||
                  player->pc2004_action_state == 12)) ?
                player->pc2004_action_state : (player->ball_form ? 8 : 12);
            player->animation_landing_valid = isfinite(player->vel.y);
        }
        player->pos.y = surface.ground_y + 1.0f;
        player->vel.y = 0.0f;
        player->grounded = true;
        player->jump_ticks = 0;
        if (player->action_state == SA1_ACTION_AIRBORNE) {
            player->action_state = SA1_ACTION_GROUNDED;
            player->ball_form = false;
        }
        if (landed_from_air) {
            player->pc2004_action_state =
                (player->animation_input_active ||
                 (player->vel.x * sinf(player->yaw) +
                  player->vel.z * cosf(player->yaw)) != 0.0f) ? 2 : 1;
            player->pc2004_action_state_valid = true;
            player->animation_airborne_request_valid = false;
        }
    } else if (!surface.valid || player->pos.y > surface.ground_y + 1.6f) {
        if (player->grounded)
            sa1_steam_sonic_begin_ground_departure(player);
        else
            player->grounded = false;
    }

    if (surface.valid) {
        player->surface_id = surface.surface_id;
        player->ground_normal = surface.normal;
        player->ground_slope_radians = surface.slope_radians;
    } else {
        player->surface_id = -1;
        player->ground_normal = (SA1Vec3){0.0f, 1.0f, 0.0f};
        player->ground_slope_radians = 0.0f;
    }
    if (!player->grounded) {
        player->action_state = SA1_ACTION_AIRBORNE;
    } else if (player->action_state != SA1_ACTION_SPINDASH_CHARGE &&
               player->action_state != SA1_ACTION_ROLL) {
        player->action_state = SA1_ACTION_GROUNDED;
    }

    if (player->pos.y < world->reset_min_y ||
        player->pos.x < world->reset_min_x || player->pos.x > world->reset_max_x ||
        player->pos.z < world->reset_min_z || player->pos.z > world->reset_max_z) {
        sa1_player_reset(player, world);
        return;
    }

    float hs = sa1_horizontal_speed(player);
    if (hs > player->peak_speed) player->peak_speed = hs;
    if (player->pos.y > player->peak_y) player->peak_y = player->pos.y;
    player->sim_frame++;
}
