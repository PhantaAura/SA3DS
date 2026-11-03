#include "pc2004_motion.h"
#include "pc2004_ground_accel.h"
#include "steam_air.h"

#include <math.h>
#include <string.h>

#define SA1_PI 3.14159265358979323846f
#define SA1_TAU (2.0f * SA1_PI)
#define BAMS_FULL 65536.0f
#define BAMS_22_5 0x1000u
#define BAMS_45   0x2000u
#define BAMS_135  0x6000u
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

static SA1LocalSpeed world_to_frame(const SA1SurfaceFrame *frame,
                                    SA1Vec3 velocity) {
    SA1LocalSpeed out;
    out.forward = sa1_vec3_dot(velocity, frame->forward);
    out.vertical = sa1_vec3_dot(velocity, frame->up);
    out.side = sa1_vec3_dot(velocity, frame->right);
    return out;
}

static SA1Vec3 frame_to_world(const SA1SurfaceFrame *frame,
                              SA1LocalSpeed speed) {
    SA1Vec3 out = sa1_vec3_scale(frame->forward, speed.forward);
    out = sa1_vec3_add(out, sa1_vec3_scale(frame->up, speed.vertical));
    return sa1_vec3_add(out, sa1_vec3_scale(frame->right, speed.side));
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
    /* 0x443CCE..0x443D23, status & 3 == 0. Both coefficients are
     * independent f32 constants, not the grounded helper's 0.99 branch. */
    SA1PC2004LocalVector blended=sa1_pc2004_turn_velocity(
        (SA1PC2004LocalVector){old_local.forward,old_local.vertical,old_local.side},
        (SA1PC2004LocalVector){transformed.forward,transformed.vertical,transformed.side},
        SA1_PC2004_TURN_GENERAL,player->grounded,1,1.0f,1.0f);
    transformed=(SA1LocalSpeed){blended.forward,blended.normal,blended.side};
    return transformed;
}

void sa1_steam_sonic_begin_jump(SA1PlayerState *player) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    player->vel.y = p->jmp_y_spd;
    player->grounded = false;
    player->action_state = SA1_ACTION_AIRBORNE;
    player->pc2004_action_state = 8;
    player->pc2004_action_state_valid = true;
    player->animation_airborne_request = 14;
    player->animation_airborne_request_valid = true;
    player->ball_form = true;
    player->spindash_charge = 0.0f;
    player->spindash_ticks = 0;
    player->jump_ticks = 0;
}

void sa1_steam_sonic_begin_ground_departure(SA1PlayerState *player) {
    if(player->action_state==SA1_ACTION_HURT){player->grounded=false;return;}
    SA1ActionState previous = player->action_state;
    int ball = player->ball_form || previous == SA1_ACTION_ROLL ||
               previous == SA1_ACTION_SPINDASH_CHARGE;

    player->grounded = false;
    player->action_state = SA1_ACTION_AIRBORNE;
    player->pc2004_action_state = ball ? 8 : 12;
    player->pc2004_action_state_valid = true;
    player->animation_airborne_request = previous == SA1_ACTION_ROLL ? 15 :
        (ball ? 14 : 18);
    player->animation_airborne_request_valid = true;
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

int sa1_steam_sonic_apply_jump_hold_surface(SA1PlayerState *player,
                                            const SA1SurfaceFrame *frame,
                                            int jump_held) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    if (!frame || !frame->valid || !jump_held) return 0;

    uint32_t old_timer = player->jump_ticks;
    player->jump_ticks++;
    if (old_timer >= (uint32_t)p->jump2_timer) return 0;

    SA1LocalSpeed local = world_to_frame(frame, player->vel);
    local.vertical = (float)((double)local.vertical +
                             (double)p->jmp_addit *
                             (double)SADX_JUMP_HOLD_MULTIPLIER);
    player->vel = frame_to_world(frame, local);
    return 1;
}

static float signed_angle_on_plane(SA1Vec3 from, SA1Vec3 to, SA1Vec3 up) {
    from = sa1_vec3_normalize(from);
    to = sa1_vec3_normalize(to);
    float cosine = clampf(sa1_vec3_dot(from, to), -1.0f, 1.0f);
    float sine = sa1_vec3_dot(sa1_vec3_cross(from, to), up);
    return sa1_pc2004_turn_delta(atan2f(sine, cosine));
}

static float yaw_from_forward(SA1Vec3 forward, float fallback) {
    float horizontal2 = forward.x * forward.x + forward.z * forward.z;
    return horizontal2 > 1e-8f ? atan2f(forward.x, forward.z) : fallback;
}

static void air_step_frame(SA1PlayerState *player,
                           const SA1Input *input,
                           float camera_yaw,
                           SA1SurfaceFrame *frame,
                           SA1SteamAirDebug *debug_out) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1SADXFilteredAnalog analog =
        sa1_sadx_filter_logical_analog(input->move_x, input->move_z);
    SA1LocalSpeed local = world_to_frame(frame, player->vel);
    SA1LocalSpeed before = local;
    const SA1Vec3 gravity = {0.0f, -1.0f, 0.0f};

    /* 0x44B9F0..0x44BA53 constructs Gravity*weight plus collision force,
     * then 0x43EC00 converts that world vector through the full task angles.
     * Free air has no collision-force contribution. */
    SA1LocalSpeed accel;
    accel.forward = p->weight * sa1_vec3_dot(gravity, frame->forward) +
                    p->air_resist_air * local.forward;
    accel.vertical = p->weight * sa1_vec3_dot(gravity, frame->up) +
                     p->air_resist_y * local.vertical;
    accel.side = p->weight * sa1_vec3_dot(gravity, frame->right) +
                 p->air_resist_z * local.side;

    float force = 0.0f;
    unsigned delta_bams = 0;
    unsigned turn_step = 0;
    int air_brake = 0;
    int wide_angle_block = 0;
    int doubled_accel = 0;

    if (analog.active) {
        SA1Vec3 desired = {sinf(camera_yaw + analog.angle_radians), 0.0f,
                           cosf(camera_yaw + analog.angle_radians)};
        desired = sa1_project_on_plane(desired, frame->up);
        if (sa1_vec3_length(desired) < 1e-5f) desired = frame->forward;
        desired = sa1_vec3_normalize(desired);
        float signed_delta = signed_angle_on_plane(frame->forward, desired,
                                                   frame->up);
        delta_bams = rad_to_bams_abs(signed_delta);
        int preserve_facing = 0;

        if (local.forward > p->run_speed && delta_bams > BAMS_135) {
            force = p->air_break * analog.magnitude;
            air_brake = 1;
            preserve_facing = 1;
        } else if (delta_bams > BAMS_22_5) {
            wide_angle_block = 1;
        } else {
            force = p->air_accel * analog.magnitude;
            /* The original test is the local movement-direction Y field.
             * In the retained task frame this is the forward basis Y, not
             * world vertical velocity. */
            if (frame->forward.y < 0.0f) {
                force *= 2.0f;
                doubled_accel = 1;
            }
        }

        turn_step = steam_general_turn_step(delta_bams);
        if (!preserve_facing && turn_step) {
            float maximum = bams_to_rad(turn_step);
            float step = clampf(signed_delta, -maximum, maximum);
            SA1Vec3 old_world = frame_to_world(frame, local);
            SA1LocalSpeed old_local = local;
            frame->forward = sa1_vec3_normalize(
                sa1_rotate_around_axis(frame->forward, frame->up, step));
            frame->right = sa1_vec3_normalize(
                sa1_vec3_cross(frame->up, frame->forward));
            frame->forward = sa1_vec3_normalize(
                sa1_vec3_cross(frame->right, frame->up));
            SA1LocalSpeed transformed = world_to_frame(frame, old_world);
            SA1PC2004LocalVector blended=sa1_pc2004_turn_velocity(
                (SA1PC2004LocalVector){old_local.forward,old_local.vertical,old_local.side},
                (SA1PC2004LocalVector){transformed.forward,transformed.vertical,transformed.side},
                SA1_PC2004_TURN_GENERAL,player->grounded,1,frame->up.y,1.0f);
            local=(SA1LocalSpeed){blended.forward,blended.normal,blended.side};
        }
        accel.forward += force;
    }

    SA1PC2004LocalVector integrated;
    sa1_pc2004_pgetspeed_local(
        (SA1PC2004LocalVector){local.forward,local.vertical,local.side},
        (SA1PC2004LocalVector){accel.forward,accel.vertical,accel.side},
        analog.active, frame->up.y, 0, p, &integrated);
    local=(SA1LocalSpeed){integrated.forward,integrated.normal,integrated.side};
    local.forward = clampf(local.forward, -p->lim_h_spd, p->lim_h_spd);
    local.side = clampf(local.side, -p->lim_h_spd, p->lim_h_spd);
    local.vertical = clampf(local.vertical, -p->lim_v_spd, p->lim_v_spd);
    player->vel = frame_to_world(frame, local);
    player->yaw = yaw_from_forward(frame->forward, player->yaw);

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

void sa1_steam_sonic_air_step_surface(SA1PlayerState *player,
                                      const SA1Input *input,
                                      float camera_yaw,
                                      SA1SurfaceFrame *frame,
                                      SA1SteamAirDebug *debug_out) {
    if (!frame || !frame->valid) {
        sa1_steam_sonic_air_step(player, input, camera_yaw, debug_out);
        return;
    }
    air_step_frame(player, input, camera_yaw, frame, debug_out);
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

    SA1PC2004LocalVector integrated;
    sa1_pc2004_pgetspeed_local(
        (SA1PC2004LocalVector){local.forward,local.vertical,local.side},
        (SA1PC2004LocalVector){accel.forward,accel.vertical,accel.side},
        analog.active, 1.0f, 0, p, &integrated);
    local=(SA1LocalSpeed){integrated.forward,integrated.normal,integrated.side};

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

void sa1_steam_sonic_noinput_step_surface(SA1PlayerState *player,
                                         const SA1Input *input,
                                         const SA1SurfaceFrame *frame) {
    const SA1SteamPlayerParameters *p=&SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1LocalSpeed speed=world_to_frame(frame,player->vel);
    SA1PC2004LocalVector local={speed.forward,speed.vertical,speed.side};
    SA1PC2004LocalVector force={-p->weight*frame->forward.y,
        -p->weight*frame->up.y,-p->weight*frame->right.y},add,next;
    SA1PC2004MotionContext context={player->grounded,player->ball_form,0,frame->up.y,1.0f};
    sa1_pc2004_noinput_add_velocity(local,force,&context,p,&add);
    SA1SADXFilteredAnalog analog=sa1_sadx_filter_logical_analog(input->move_x,input->move_z);
    sa1_pc2004_pgetspeed_local(local,add,analog.active,frame->up.y,0,p,&next);
    speed=(SA1LocalSpeed){clampf(next.forward,-p->lim_h_spd,p->lim_h_spd),
        clampf(next.normal,-p->lim_v_spd,p->lim_v_spd),
        clampf(next.side,-p->lim_h_spd,p->lim_h_spd)};
    player->vel=frame_to_world(frame,speed);
}
