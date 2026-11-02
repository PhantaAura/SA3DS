#include "pc2004_ground_accel.h"
#include "steam_ball.h"
#include "pc2004_motion.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

#define SA1_PI 3.14159265358979323846f
#define SA1_TAU (2.0f * SA1_PI)
#define BAMS_FULL 65536.0f

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
    float scaled = sa1_pc2004_turn_delta(radians) / SA1_TAU * BAMS_FULL;
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

static void general_ball_turn(SA1PlayerState *player, SA1LocalSpeed *local,
                             const SA1Input *input, float camera_yaw,
                             unsigned *step_out) {
    SA1SADXFilteredAnalog analog =
        sa1_sadx_filter_logical_analog(input->move_x, input->move_z);
    if (!analog.active) {
        if (step_out) *step_out = 0;
        return;
    }

    float target = wrap_angle(camera_yaw + analog.angle_radians);
    int delta = radians_to_signed_bams(target - player->yaw);
    unsigned magnitude = (unsigned)abs(delta);
    unsigned limit = magnitude > 0x2000u ? 0x800u :
                     magnitude > 0x1000u ? magnitude >> 2 : magnitude >> 3;
    SA1Vec3 old_world = local_to_world(player->yaw, *local);
    SA1LocalSpeed old_local = *local;
    int step = clamp_signed_bams(delta, (int)limit);
    player->yaw = wrap_angle(player->yaw + bams_to_radians(step));

    SA1LocalSpeed transformed = world_to_local(player->yaw, old_world);
    SA1PC2004LocalVector blended = sa1_pc2004_ground_turn_velocity(
        (SA1PC2004LocalVector){old_local.forward,old_local.vertical,old_local.side},
        (SA1PC2004LocalVector){transformed.forward,transformed.vertical,transformed.side},
        SA1_PC2004_TURN_GENERAL, 1, 1.0f, 1.0f);
    *local=(SA1LocalSpeed){blended.forward,blended.normal,blended.side};
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
    general_ball_turn(player, &local, input, camera_yaw, turn_step_out);
    SA1PC2004LocalVector speed = {local.forward, 0.0f, local.side};
    SA1PC2004LocalVector force = {0.0f, -p->weight, 0.0f};
    SA1PC2004MotionContext context = {1, 0, 0, 1.0f, 1.0f};
    SA1PC2004LocalVector add = sa1_pc2004_pgetbreak_add_velocity(
        speed, force, &context, p, NULL);
    SA1PC2004LocalVector integrated;
    sa1_pc2004_pgetspeed_local(speed,add,
        sa1_sadx_filter_logical_analog(input->move_x,input->move_z).active,
        1.0f,0,p,&integrated);
    local=(SA1LocalSpeed){integrated.forward,integrated.normal,integrated.side};
    player->vel = local_to_world(player->yaw, local);
}

static void roll_ground_motion(SA1PlayerState *player,
                               const SA1Input *input, float camera_yaw,
                               unsigned *turn_step_out) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1LocalSpeed local = world_to_local(player->yaw, player->vel);

    /* PGetRotation precedes PGetInertia for action5, including the drag
     * preparation. This is the default M1/no-pending-impulse flat context. */
    general_ball_turn(player, &local, input, camera_yaw, turn_step_out);
    SA1PC2004LocalVector speed={local.forward,0.0f,local.side};
    SA1PC2004LocalVector force={0.0f,-p->weight,0.0f};
    SA1PC2004MotionContext context={1,1,0,1.0f,1.0f};
    SA1PC2004LocalVector add=sa1_pc2004_pgetinertia_add_velocity(
        speed,force,&context,p,NULL);
    SA1PC2004LocalVector integrated;
    sa1_pc2004_pgetspeed_local(speed,add,
        sa1_sadx_filter_logical_analog(input->move_x,input->move_z).active,
        1.0f,0,p,&integrated);
    local=(SA1LocalSpeed){integrated.forward,integrated.normal,integrated.side};
    player->vel=local_to_world(player->yaw,local);
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
            player->pc2004_action_state_valid = false;
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
            player->pc2004_action_state = 5;
            player->pc2004_action_state_valid = true;
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
            player->pc2004_action_state = local.forward <= 0.0f ? 1 : 2;
            player->pc2004_action_state_valid = true;
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
        player->pc2004_action_state = 4;
        player->pc2004_action_state_valid = true;
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
