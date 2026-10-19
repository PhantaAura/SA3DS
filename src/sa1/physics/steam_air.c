#include "steam_air.h"

#include <math.h>
#include <string.h>

#define SA1_PI 3.14159265358979323846f
#define SA1_TAU (2.0f * SA1_PI)
#define BAMS_FULL 65536.0f
#define BAMS_22_5 0x1000u
#define BAMS_45   0x2000u
#define BAMS_135  0x6000u
#define SADX_UPRIGHT_TURN_BLEND 0.99f
#define SADX_JUMP_HOLD_MULTIPLIER 0.800000011920929f

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

/* Steam general turn helper 0x004E2480, flat/upright specialization. */
static unsigned steam_general_turn_step(unsigned delta_bams) {
    if (delta_bams > BAMS_45) return 0x0800u;
    if (delta_bams > BAMS_22_5) return delta_bams >> 2;
    return delta_bams >> 3;
}

static SA1LocalSpeed apply_general_turn(SA1PlayerState *player,
                                        SA1LocalSpeed old_local,
                                        float target,
                                        unsigned step_bams) {
    SA1Vec3 old_world = local_to_world(player->yaw, old_local);
    player->yaw = approach_angle_bams(player->yaw, target, step_bams);
    SA1LocalSpeed transformed = world_to_local(player->yaw, old_world);
    transformed.forward = transformed.forward * SADX_UPRIGHT_TURN_BLEND +
                          old_local.forward * (1.0f - SADX_UPRIGHT_TURN_BLEND);
    transformed.vertical = transformed.vertical * SADX_UPRIGHT_TURN_BLEND +
                           old_local.vertical * (1.0f - SADX_UPRIGHT_TURN_BLEND);
    transformed.side = transformed.side * SADX_UPRIGHT_TURN_BLEND +
                       old_local.side * (1.0f - SADX_UPRIGHT_TURN_BLEND);
    return transformed;
}

static float integrate_axis_no_cross(float old_speed, float accel) {
    float next = old_speed + accel;
    if ((old_speed > 0.0f && next < 0.0f) ||
        (old_speed < 0.0f && next > 0.0f))
        next = 0.0f;
    return next;
}

void sa1_steam_sonic_begin_jump(SA1PlayerState *player) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    player->vel.y = p->jmp_y_spd;
    player->grounded = false;
    player->action_state = SA1_ACTION_AIRBORNE;
    player->ball_form = true;
    player->spindash_charge = 0.0f;
    player->spindash_ticks = 0;
    player->jump_ticks = 0;
}

int sa1_steam_sonic_apply_jump_hold(SA1PlayerState *player, int jump_held) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    if (!jump_held) return 0;

    /* 0x004DBA90 compares the pre-increment timer against jump2_timer. */
    uint32_t old_timer = player->jump_ticks;
    player->jump_ticks++;
    if (old_timer >= (uint32_t)p->jump2_timer) return 0;

    player->vel.y = (float)((double)player->vel.y +
                            (double)p->jmp_addit *
                            (double)SADX_JUMP_HOLD_MULTIPLIER);
    return 1;
}

void sa1_steam_sonic_air_step(SA1PlayerState *player,
                              const SA1Input *input,
                              float camera_yaw,
                              SA1SteamAirDebug *debug_out) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1SADXFilteredAnalog analog =
        sa1_sadx_filter_logical_analog(input->move_x, input->move_z);
    SA1LocalSpeed local = world_to_local(player->yaw, player->vel);
    SA1LocalSpeed before = local;

    /* Supported flat-world specialization of 0x004DDEC0:
     * Gravity is (0,-1,0), so Gravity * weight contributes -weight to Y.
     * Collision-force input is zero while free airborne. */
    SA1LocalSpeed accel;
    accel.forward = p->air_resist_air * local.forward;
    accel.vertical = -p->weight + p->air_resist_y * local.vertical;
    accel.side = p->air_resist_z * local.side;

    float force = 0.0f;
    unsigned delta_bams = 0;
    unsigned turn_step = 0;
    int air_brake = 0;
    int wide_angle_block = 0;
    int doubled_accel = 0;

    if (analog.active) {
        float desired = wrap_angle(camera_yaw + analog.angle_radians);
        delta_bams = rad_to_bams_abs(desired - player->yaw);
        float turn_target = desired;

        /* Exact Steam branch shape: forward >= run_speed and >135-degree
         * opposing input selects air_break. The routine does not overwrite
         * Forward.y in this branch, so the flat candidate preserves current
         * facing as the turn target for that frame. */
        if (local.forward > p->run_speed && delta_bams > BAMS_135) {
            force = p->air_break * analog.magnitude;
            air_brake = 1;
            turn_target = player->yaw;
        } else if (delta_bams > BAMS_22_5) {
            /* Steam changes the requested facing but supplies no air thrust
             * for angles wider than 22.5 degrees. */
            force = 0.0f;
            wide_angle_block = 1;
        } else {
            force = p->air_accel * analog.magnitude;
            /* 0x004DDEC0 doubles air_accel while its movement-direction Y is
             * non-positive. World vertical velocity is the flat-lab
             * equivalent until surface-relative motion lands in 0.0.8.7. */
            if (player->vel.y < 0.0f) {
                force *= 2.0f;
                doubled_accel = 1;
            }
        }

        turn_step = steam_general_turn_step(delta_bams);
        local = apply_general_turn(player, local, turn_target, turn_step);

        /* Turning transforms local velocity. Resistance was computed before
         * the helper in the executable, so keep accel as already calculated. */
        accel.forward += force;
    }

    local.forward = integrate_axis_no_cross(local.forward, accel.forward);
    local.vertical += accel.vertical;
    local.side = integrate_axis_no_cross(local.side, accel.side);

    local.forward = clampf(local.forward, -p->lim_h_spd, p->lim_h_spd);
    local.side = clampf(local.side, -p->lim_h_spd, p->lim_h_spd);
    local.vertical = clampf(local.vertical, -p->lim_v_spd, p->lim_v_spd);
    player->vel = local_to_world(player->yaw, local);

    if (debug_out) {
        memset(debug_out, 0, sizeof(*debug_out));
        debug_out->forward_speed_before = before.forward;
        debug_out->vertical_speed_before = before.vertical;
        debug_out->side_speed_before = before.side;
        debug_out->forward_speed_after = local.forward;
        debug_out->vertical_speed_after = local.vertical;
        debug_out->side_speed_after = local.side;
        debug_out->accel_forward = accel.forward;
        debug_out->accel_vertical = accel.vertical;
        debug_out->accel_side = accel.side;
        debug_out->requested_force = force;
        debug_out->filtered_input_magnitude = analog.magnitude;
        debug_out->angle_delta_bams = delta_bams;
        debug_out->turn_step_bams = turn_step;
        debug_out->had_analog = analog.active;
        debug_out->used_air_brake = air_brake;
        debug_out->blocked_wide_angle_accel = wide_angle_block;
        debug_out->doubled_air_accel = doubled_accel;
    }
}
