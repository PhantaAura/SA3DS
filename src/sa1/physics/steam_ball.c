#include "steam_ball.h"
#include "pc2004_motion.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

#define SA1_PI 3.14159265358979323846f
#define SA1_TAU (2.0f * SA1_PI)
#define BAMS_FULL 65536.0f
#define SPINDASH_TURN_LIMIT 0x1000
#define ROLL_TURN_LIMIT     0x0800

typedef struct {
    float forward;
    float vertical;
    float side;
} SA1LocalSpeed;

static float wrap_angle(float a) {
    while (a > SA1_PI) a -= SA1_TAU;
    while (a < -SA1_PI) a += SA1_TAU;
    return a;
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

static int radians_to_signed_bams(float radians) {
    float scaled = wrap_angle(radians) / SA1_TAU * BAMS_FULL;
    return (int)lroundf(scaled);
}

static float bams_to_radians(int bams) {
    return ((float)bams / BAMS_FULL) * SA1_TAU;
}

static int clamp_signed_bams(int value, int limit) {
    if (value > limit) return limit;
    if (value < -limit) return -limit;
    return value;
}

static void direct_ball_turn(SA1PlayerState *player, SA1LocalSpeed *local,
                             const SA1Input *input, float camera_yaw,
                             int turn_limit_bams, unsigned *step_out) {
    SA1SADXFilteredAnalog analog =
        sa1_sadx_filter_logical_analog(input->move_x, input->move_z);
    if (!analog.active) {
        if (step_out) *step_out = 0;
        return;
    }

    float target = wrap_angle(camera_yaw + analog.angle_radians);
    int delta = radians_to_signed_bams(target - player->yaw);
    int step = clamp_signed_bams(delta, turn_limit_bams);
    player->yaw = wrap_angle(player->yaw + bams_to_radians(step));

    /* Ball movement rotates the player-local frame directly. This keeps local
     * forward/side speed while changing its world-space direction, matching
     * the structure used by the public SA1-style roll implementation. */
    player->vel = local_to_world(player->yaw, *local);
    if (step_out) *step_out = (unsigned)abs(step);
}

static void charge_ground_motion(SA1PlayerState *player,
                                 const SA1Input *input, float camera_yaw,
                                 unsigned *turn_step_out) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1LocalSpeed local = world_to_local(player->yaw, player->vel);

    /* Same verified PGetRotation -> PGetBreak -> PGetSpeed specialization
     * already used by surface_ball_motion, with upright local gravity and
     * zero dynamic collision force. Do not substitute a constant friction
     * subtraction for PGetBreak's run_break/normal-force arbitration. */
    direct_ball_turn(player, &local, input, camera_yaw,
                     SPINDASH_TURN_LIMIT, turn_step_out);
    SA1PC2004LocalVector speed = {local.forward, 0.0f, local.side};
    SA1PC2004LocalVector force = {0.0f, -p->weight, 0.0f};
    SA1PC2004MotionContext context = {1, 0, 0, 1.0f, 1.0f};
    SA1PC2004LocalVector add = sa1_pc2004_pgetbreak_add_velocity(
        speed, force, &context, p, NULL);
    local.forward = sa1_pc2004_integrate_axis_no_cross(local.forward, add.forward, NULL);
    local.side = sa1_pc2004_integrate_axis_no_cross(local.side, add.side, NULL);
    local.vertical = 0.0f;
    player->vel = local_to_world(player->yaw, local);
}

static void roll_ground_motion(SA1PlayerState *player,
                               const SA1Input *input, float camera_yaw,
                               unsigned *turn_step_out) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1LocalSpeed local = world_to_local(player->yaw, player->vel);

    /* Flat specialization of roll movement. On a flat surface the slope
     * gravity terms vanish; RollDecel/air_resist remains the forward decay. */
    local.forward += local.forward * p->air_resist;
    local.side += local.side * p->air_resist_z;
    local.vertical = 0.0f;
    direct_ball_turn(player, &local, input, camera_yaw,
                     ROLL_TURN_LIMIT, turn_step_out);
    player->vel = local_to_world(player->yaw, local);
}

int sa1_steam_sonic_ball_ground_step(SA1PlayerState *player,
                                     const SA1Input *input,
                                     float camera_yaw,
                                     SA1SteamBallDebug *debug_out) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1SteamBallDebug local_debug;
    SA1SteamBallDebug *dbg = debug_out ? debug_out : &local_debug;
    memset(dbg, 0, sizeof(*dbg));

    SA1LocalSpeed local = world_to_local(player->yaw, player->vel);
    dbg->forward_speed_before = local.forward;
    dbg->charge_before = player->spindash_charge;

    int requested = input->action_pressed || input->spindash_shortcut_pressed;
    int held = input->action_held || input->spindash_shortcut_held;
    dbg->used_shortcut_request = input->spindash_shortcut_pressed ||
                                 input->spindash_shortcut_held ||
                                 input->spindash_shortcut_released;

    if (player->action_state == SA1_ACTION_SPINDASH_CHARGE) {
        /* 0x49AA34 calls Sonic_Act1 before the timer increment at 0x49AB01
         * and movement dispatch at 0x49AB29. Transitions inspect the timer
         * left by the previous tick, then dispatch the newly selected action. */
        if (sa1_sadx_spindash_timeout_reached(player->spindash_ticks)) {
            player->spindash_ticks = 0;
            player->action_state = SA1_ACTION_GROUNDED;
            player->ball_form = false;
            dbg->timed_out_charge = 1;
            return 0;
        } else if (held) {
            player->spindash_charge =
                sa1_sadx_spindash_charge_held_tick(player->spindash_charge);
            dbg->charged_this_tick = 1;
            player->spindash_ticks++;
            charge_ground_motion(player, input, camera_yaw, &dbg->turn_step_bams);
        } else {
            SA1LocalSpeed release = world_to_local(player->yaw, player->vel);
            release.forward = player->spindash_charge;
            player->vel = local_to_world(player->yaw, release);
            player->action_state = SA1_ACTION_ROLL;
            player->ball_form = true;
            player->spindash_charge = 0.0f;
            player->spindash_ticks = 1; /* ball-status increment after release */
            dbg->released_to_roll = 1;
            roll_ground_motion(player, input, camera_yaw, &dbg->turn_step_bams);
        }

        dbg->charge_after = player->spindash_charge;
        dbg->forward_speed_after = world_to_local(player->yaw, player->vel).forward;
        return 1;
    }

    if (player->action_state == SA1_ACTION_ROLL) {
        /* PC2004 0x4930D0, called by action-5 transitions at 0x497458:
         * signed Speed.x < run_speed exits before this tick's movement.
         * In particular, a negative high-magnitude roll cannot persist. */
        if (local.forward < p->run_speed || requested) {
            player->action_state = SA1_ACTION_GROUNDED;
            player->ball_form = false;
            dbg->exited_roll = 1;
            dbg->forward_speed_after = local.forward;
            return 0;
        }

        roll_ground_motion(player, input, camera_yaw, &dbg->turn_step_bams);
        player->spindash_ticks++;
        local = world_to_local(player->yaw, player->vel);

        dbg->charge_after = player->spindash_charge;
        dbg->forward_speed_after = local.forward;
        return 1;
    }

    if (requested && player->grounded) {
        player->spindash_charge =
            sa1_sadx_spindash_initial_charge(local.forward);
        player->spindash_ticks = 1; /* entry resets, then main increments */
        player->action_state = SA1_ACTION_SPINDASH_CHARGE;
        player->ball_form = true;
        dbg->entered_charge = 1;
        charge_ground_motion(player, input, camera_yaw, &dbg->turn_step_bams);
        dbg->charge_after = player->spindash_charge;
        dbg->forward_speed_after = world_to_local(player->yaw, player->vel).forward;
        return 1;
    }

    dbg->charge_after = player->spindash_charge;
    dbg->forward_speed_after = local.forward;
    return 0;
}
