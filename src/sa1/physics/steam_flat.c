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
#define SADX_LOW_SPEED_ZERO_EPS 0.051f
#define SADX_PGETSPEED_IDLE_FACTOR 0.87f

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
    float diff = wrap_angle(target - angle);
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

/* Steam helper 0x004E2800. Above dash speed the maximum turn step shrinks as
 * 0x100 - round(sqrt((speed - dash_speed) / 16) * 256). Sonic's horizontal
 * limit keeps the result positive for the normal parameter range. */
static unsigned steam_slow_turn_step(float speed,
                                     const SA1SteamPlayerParameters *p) {
    int step = 0x100;
    if (speed >= p->dash_speed) {
        float excess = (speed - p->dash_speed) / 16.0f;
        int reduction = (int)lroundf(sqrtf(excess > 0.0f ? excess : 0.0f) * 256.0f);
        step -= reduction;
        if (step < 1) step = 1;
    }
    return (unsigned)step;
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

/* Flat supported ground has no net local-Y acceleration: the contact reaction
 * cancels gravity before the friction tail. This is the flat specialization of
 * PGetAcceleration's Gravity*weight + collision-vector term. */
static float flat_forward_accel(float add_x, float force_x, float speed_x,
                                int has_analog,
                                const SA1SteamPlayerParameters *p) {
    (void)p;

    if (speed_x == 0.0f) {
        add_x += force_x;
        if (!has_analog && fabsf(add_x) < SADX_LOW_SPEED_ZERO_EPS)
            add_x = 0.0f;
        return add_x;
    }

    if (force_x < 0.0f) {
        if (speed_x <= 0.0f) {
            if (!has_analog && speed_x <= p->jog_speed &&
                fabsf(add_x) < SADX_LOW_SPEED_ZERO_EPS)
                return 0.0f;
            /* With zero net normal acceleration on flat supported ground the
             * lim_frict term is zero; negative force remains valid. */
            return add_x + force_x;
        }

        float v = add_x + force_x;
        if (force_x * v < 0.0f) v = 0.0f;
        return v;
    }

    if (!has_analog && speed_x <= p->jog_speed &&
        fabsf(add_x) < SADX_LOW_SPEED_ZERO_EPS)
        return 0.0f;
    return add_x + force_x;
}

static float flat_side_accel(float side_speed,
                             const SA1SteamPlayerParameters *p) {
    /* On flat supported ground the normal-force friction term is zero. The
     * verified air_resist_z term remains and rapidly damps player-local side
     * velocity generated by steering. */
    return side_speed * p->air_resist_z;
}

/* Steam 0x004E2950 first integrates local acceleration, then prevents forward
 * speed from crossing through zero and performs a no-input low-speed stop. */
static float integrate_forward(float old_speed, float accel, int has_analog,
                               const SA1SteamPlayerParameters *p,
                               int *crossed_zero) {
    float next = old_speed + accel;
    int crossed = 0;

    if ((old_speed > 0.0f && next < 0.0f) ||
        (old_speed < 0.0f && next > 0.0f)) {
        next = 0.0f;
        crossed = 1;
    }

    if (!has_analog && fabsf(next) <= p->jog_speed * SADX_PGETSPEED_IDLE_FACTOR)
        next = 0.0f;

    if (crossed_zero) *crossed_zero = crossed;
    return clampf(next, -p->lim_h_spd, p->lim_h_spd);
}

static float integrate_side(float old_speed, float accel,
                            const SA1SteamPlayerParameters *p,
                            int *stopped_at_threshold) {
    float next = old_speed + accel;
    float threshold = p->jog_speed * SADX_PGETSPEED_IDLE_FACTOR;
    int stopped = 0;

    /* PC2004 PGetSpeed 0x0044403A..0x004440E1 applies its
     * 0.87*jog_speed deadband to local-Z as well as forward speed. It snaps
     * lateral inertia when damping crosses into that band, or when an already
     * small lateral value receives no acceleration. */
    if (old_speed > 0.0f) {
        if ((old_speed >= threshold && next < threshold) ||
            (old_speed < threshold && old_speed == next)) {
            next = 0.0f;
            stopped = 1;
        }
    } else if (old_speed < 0.0f) {
        if ((old_speed <= -threshold && next > -threshold) ||
            (old_speed > -threshold && old_speed == next)) {
            next = 0.0f;
            stopped = 1;
        }
    }
    if (stopped_at_threshold) *stopped_at_threshold = stopped;
    if ((old_speed > 0.0f && next < 0.0f) ||
        (old_speed < 0.0f && next > 0.0f))
        next = 0.0f;
    return clampf(next, -p->lim_h_spd, p->lim_h_spd);
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

    /* PGetAcceleration emits local acceleration. On supported flat ground the
     * vertical contact force cancels gravity, leaving no net Y contribution. */
    float accel_forward = flat_forward_accel(resistance, force, local.forward,
                                             analog.active, p);
    float accel_side = flat_side_accel(local.side, p);

    local.forward = integrate_forward(local.forward, accel_forward,
                                      analog.active, p, &crossed_zero);
    local.side = integrate_side(local.side, accel_side, p, &side_stopped);
    local.vertical = clampf(local.vertical, -p->lim_v_spd, p->lim_v_spd);
    player->vel = local_to_world(player->yaw, local);

    /* PC2004 PGetAcceleration clears EntityData2+0x38 before this path and
     * only writes it when a nonzero ground-friction limit clips forward
     * force. The supported flat/upright specialization has zero net local-Y
     * contact acceleration, so that residual remains exactly zero. */
    player->animation_accel_residual = 0.0f;
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

    /* Mirrors the per-tick clear at PC2004 0x00446F10. A path marks the value
     * valid only where its residual behavior has been recovered. */
    player->animation_accel_residual = 0.0f;
    player->animation_accel_residual_valid = false;

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
                steam_flat_horizontal(player, input, camera_yaw, debug_out);
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
        if (!just_jumped)
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
        player->pos.y = surface.ground_y + 1.0f;
        player->vel.y = 0.0f;
        player->grounded = true;
        player->jump_ticks = 0;
        if (player->action_state == SA1_ACTION_AIRBORNE) {
            player->action_state = SA1_ACTION_GROUNDED;
            player->ball_form = false;
        }
    } else if (!surface.valid || player->pos.y > surface.ground_y + 1.6f) {
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
