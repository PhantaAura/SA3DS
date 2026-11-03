#include "pc2004_task_frame.h"
#include "pc2004_ground_accel.h"
#include "pc2004_air_attack.h"
#include "steam_surface.h"

#include "steam_air.h"
#include "steam_ball.h"
#include "pc2004_motion.h"
#include "pc2004_collision_work.h"
#include "steam_surface_evidence.h"
#include "surface_frame.h"

#include <math.h>
#include <string.h>

#define SA1_PI 3.14159265358979323846f
#define SA1_TAU (2.0f * SA1_PI)
#define BAMS_FULL 65536.0f
#define BAMS_22_5 0x1000u
#define BAMS_45   0x2000u

/* Movement-Lab support-center offset. This is deliberately not labeled as
 * Sonic's retail collision radius; stage import/collision will eventually
 * provide the exact support shape. */
#define SA1_SUPPORT_OFFSET_CANDIDATE 1.0f
#define SA1_SUPPORT_QUERY_RANGE_CANDIDATE 2.35f
#define SA1_NORMAL_CONTINUITY_MIN_CANDIDATE -0.10f
#define SA1_UPRIGHT_EPS 0.99995f

typedef struct {
    float forward;
    float side;
    float normal;
} SurfaceSpeed;

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float wrap_angle(float a) {
    while (a > SA1_PI) a -= SA1_TAU;
    while (a < -SA1_PI) a += SA1_TAU;
    return a;
}

static unsigned radians_to_bams_abs(float radians) {
    float turns = fabsf(wrap_angle(radians)) / SA1_TAU;
    unsigned v = (unsigned)(turns * BAMS_FULL + 0.5f);
    return v > 0x8000u ? 0x8000u : v;
}

static float bams_to_rad(unsigned bams) {
    return ((float)bams / BAMS_FULL) * SA1_TAU;
}

static unsigned general_turn_step(unsigned delta_bams) {
    if (delta_bams > BAMS_45) return 0x0800u;
    if (delta_bams > BAMS_22_5) return delta_bams >> 2;
    return delta_bams >> 3;
}

static float select_analog_force(float speed, float magnitude,
                                 const SA1SteamPlayerParameters *p) {
    if (speed < p->max_x_spd) {
        if (speed < p->jog_speed) {
            if (magnitude > 0.5f) return magnitude * p->run_accel;
            if (speed >= p->jog_speed * 0.4f) return 0.0f;
            return magnitude * p->run_accel;
        }
        if (speed < p->run_speed)
            return magnitude * p->run_accel;
        if (speed < p->rush_speed) {
            float force = magnitude * p->run_accel;
            if (magnitude <= 0.9f) force *= 0.3f;
            return force;
        }
        return magnitude * p->run_accel;
    }
    return magnitude * p->run_accel * 0.4f;
}

static SA1Vec3 horizontal_from_yaw(float yaw) {
    return (SA1Vec3){sinf(yaw), 0.0f, cosf(yaw)};
}

static float yaw_from_forward(SA1Vec3 forward, float fallback) {
    float h2 = forward.x*forward.x + forward.z*forward.z;
    if (h2 < 1e-8f) return fallback;
    return atan2f(forward.x, forward.z);
}

static SurfaceSpeed world_to_surface(const SA1SurfaceFrame *frame, SA1Vec3 v) {
    SurfaceSpeed s;
    s.forward = sa1_vec3_dot(v, frame->forward);
    s.side = sa1_vec3_dot(v, frame->right);
    s.normal = sa1_vec3_dot(v, frame->up);
    return s;
}

static SA1Vec3 surface_to_world(const SA1SurfaceFrame *frame, SurfaceSpeed s) {
    SA1Vec3 v = sa1_vec3_scale(frame->forward, s.forward);
    v = sa1_vec3_add(v, sa1_vec3_scale(frame->right, s.side));
    v = sa1_vec3_add(v, sa1_vec3_scale(frame->up, s.normal));
    return v;
}

static SA1SurfaceFrame player_frame(const SA1PlayerState *player) {
    SA1Vec3 preferred = player->surface_forward;
    if (sa1_vec3_length(preferred) < 0.5f)
        preferred = horizontal_from_yaw(player->yaw);
    return sa1_surface_frame_make(player->ground_normal, preferred);
}

static float signed_angle_on_plane(SA1Vec3 from, SA1Vec3 to, SA1Vec3 up) {
    from = sa1_vec3_normalize(from);
    to = sa1_vec3_normalize(to);
    float c = clampf(sa1_vec3_dot(from, to), -1.0f, 1.0f);
    float s = sa1_vec3_dot(sa1_vec3_cross(from, to), up);
    return sa1_pc2004_turn_delta(atan2f(s, c));
}

static void rotate_frame_toward(SA1SurfaceFrame *frame,
                                SurfaceSpeed *speed,
                                SA1Vec3 desired,
                                unsigned step_bams,
                                SA1PC2004GroundTurnKind kind,int grounded) {
    float delta = signed_angle_on_plane(frame->forward, desired, frame->up);
    float max_step = bams_to_rad(step_bams);
    float step = clampf(delta, -max_step, max_step);
    SA1Vec3 old_world = surface_to_world(frame, *speed);
    SurfaceSpeed old_local = *speed;

    frame->forward = sa1_vec3_normalize(
        sa1_rotate_around_axis(frame->forward, frame->up, step));
    frame->right = sa1_vec3_normalize(sa1_vec3_cross(frame->up, frame->forward));
    frame->forward = sa1_vec3_normalize(sa1_vec3_cross(frame->right, frame->up));

    SurfaceSpeed transformed = world_to_surface(frame, old_world);
    SA1PC2004LocalVector blended = sa1_pc2004_turn_velocity(
        (SA1PC2004LocalVector){old_local.forward, old_local.normal, old_local.side},
        (SA1PC2004LocalVector){transformed.forward, transformed.normal, transformed.side},
        kind, grounded, 1, frame->up.y,
        1.0f /* Existing ordinary default-friction specialization; contact mapping pending. */);
    *speed = (SurfaceSpeed){blended.forward, blended.side, blended.normal};
}

static SA1Vec3 desired_surface_direction(const SA1SurfaceFrame *frame,
                                         float camera_yaw,
                                         const SA1SADXFilteredAnalog *analog) {
    SA1Vec3 desired = horizontal_from_yaw(camera_yaw + analog->angle_radians);
    desired = sa1_project_on_plane(desired, frame->up);
    if (sa1_vec3_length(desired) < 1e-4f)
        desired = frame->forward;
    return sa1_vec3_normalize(desired);
}

static void sync_player_frame(SA1PlayerState *player,
                              const SA1SurfaceFrame *frame,
                              const SA1Contact *contact) {
    player->ground_normal = frame->up;
    player->surface_forward = frame->forward;
    player->surface_right = frame->right;
    player->ground_slope_radians = sa1_surface_slope_radians(frame->up);
    player->yaw = yaw_from_forward(frame->forward, player->yaw);
    if (contact) {
        player->surface_id = contact->surface_id;
        player->surface_flags = contact->surface_flags;
        player->support_distance = contact->signed_distance;
    }
}

typedef enum {
    SUPPORT_QUERY_INITIAL = 0,
    SUPPORT_QUERY_POST_MOTION = 1
} SupportQueryPhase;

static int refresh_support(SA1PlayerState *player,
                           const SA1PhysicsWorld *world,
                           SA1SurfaceFrame *frame,
                           SA1SteamSurfaceDebug *dbg,
                           SA1PC2004CollisionWork *collision_work,
                           SupportQueryPhase phase) {
    SA1Vec3 probe = sa1_vec3_scale(frame->up, -1.0f);
    if (dbg) {
        dbg->support_queries++;
        if (phase == SUPPORT_QUERY_INITIAL) dbg->initial_support_queries++;
        else dbg->post_motion_support_queries++;
    }
    SA1Contact contact = sa1_world_query_contact(world, player->pos, probe,
                                                  SA1_SUPPORT_QUERY_RANGE_CANDIDATE);
    if (!contact.valid) return 0;

    float alignment = sa1_vec3_dot(frame->up, contact.normal);
    if (dbg) dbg->normal_alignment = alignment;
    if (alignment < SA1_NORMAL_CONTINUITY_MIN_CANDIDATE) return 0;

    SA1SurfaceFrame next = sa1_surface_frame_make(contact.normal, frame->forward);
    if (!next.valid) return 0;

    player->pos = sa1_vec3_add(contact.point,
                              sa1_vec3_scale(next.up, SA1_SUPPORT_OFFSET_CANDIDATE));
    SurfaceSpeed local = world_to_surface(&next, player->vel);
    local.normal = 0.0f;
    player->vel = surface_to_world(&next, local);
    *frame = next;
    sync_player_frame(player, frame, &contact);
    if (collision_work)
        sa1_pc2004_collision_work_mirror_static_contact(collision_work, &contact, 0);
    if (dbg) {
        dbg->snapped_to_support = 1;
        dbg->support_distance = contact.signed_distance;
    }
    return 1;
}

static void detach_player(SA1PlayerState *player,
                          SA1SteamSurfaceDebug *dbg) {
    sa1_steam_sonic_begin_ground_departure(player);
    if (dbg) dbg->detached = 1;
}

static void begin_surface_jump(SA1PlayerState *player,
                               const SA1SurfaceFrame *frame) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1Vec3 jump = sa1_vec3_scale(frame->up, p->jmp_y_spd);
    player->vel = sa1_vec3_add(player->vel, jump);
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

static void surface_running(SA1PlayerState *player,
                            const SA1Input *input,
                            float camera_yaw,
                            SA1SurfaceFrame *frame,
                            SA1SteamSurfaceDebug *dbg) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1SADXFilteredAnalog analog =
        sa1_sadx_filter_logical_analog(input->move_x, input->move_z);
    SurfaceSpeed s = world_to_surface(frame, player->vel);
    if (dbg) dbg->forward_speed_before = s.forward;

    float resistance = 0.0f;
    if (analog.active) {
        if (s.forward > p->max_x_spd)
            resistance = (s.forward - p->max_x_spd) * p->air_resist * 1.7f;
        else if (s.forward < 0.0f)
            resistance = p->air_resist * s.forward;
    } else if (s.forward > p->run_speed || s.forward < 0.0f) {
        resistance = p->air_resist * s.forward;
    }

    SA1Vec3 gravity_world = {0.0f, -p->weight, 0.0f};
    float g_forward = sa1_vec3_dot(gravity_world, frame->forward);
    float g_side = sa1_vec3_dot(gravity_world, frame->right);
    float weight = p->weight > 1e-6f ? p->weight : 1.0f;

    /* 0.0.8.8: replace the hand-tuned 0.0.8.7 slope factors with the
     * recovered PGetAcceleration branch shape.  The original routine scales
     * local forward/side force based on Up and speed after transforming the
     * gravity/collision force into player space.  Our contact frame supplies
     * the local axes; exact collision-force ordering remains an evidence gap. */
    float forward_scale = 1.0f, side_scale = 1.0f;
    sa1_sadx_surface_force_scales(frame->up.y, s.forward, p,
                                  &forward_scale, &side_scale);
    g_forward *= forward_scale;
    g_side *= side_scale;

    float normal_force = sa1_vec3_dot(gravity_world, frame->up) +
                         s.normal * p->air_resist_y;
    float side_resistance = s.side * p->air_resist_z;
    float unstick_bias = sa1_sadx_surface_unstick_normal_bias(
        frame->up.y, s.forward, weight);

    float force = 0.0f;
    if (analog.active) {
        SA1Vec3 desired = desired_surface_direction(frame, camera_yaw, &analog);
        float signed_delta = signed_angle_on_plane(frame->forward, desired, frame->up);
        unsigned delta_bams = radians_to_bams_abs(signed_delta);
        force = select_analog_force(s.forward, analog.magnitude, p);

        unsigned step;
        SA1PC2004GroundTurnKind kind = SA1_PC2004_TURN_GENERAL;
        if (s.forward == 0.0f && delta_bams > BAMS_22_5) {
            force = 0.0f;
            step = BAMS_45;
            kind = SA1_PC2004_TURN_QUICK;
        } else {
            float low_turn_boundary = (p->run_speed + p->jog_speed) * 0.5f;
            if (low_turn_boundary > s.forward || delta_bams <= BAMS_22_5) {
                if (s.forward >= p->jog_speed && delta_bams < BAMS_22_5) {
                    step = sa1_pc2004_slow_turn_step(s.forward, p->dash_speed);
                    kind = SA1_PC2004_TURN_SLOW;
                } else {
                    if (s.forward >= p->jog_speed && s.forward <= p->rush_speed &&
                        delta_bams > BAMS_45)
                        force *= 0.8f;
                    step = general_turn_step(delta_bams);
                }
            } else {
                force = p->slow_down;
                step = general_turn_step(delta_bams);
            }
        }
        rotate_frame_toward(frame, &s, desired, step, kind,player->grounded);
        if (dbg) dbg->turn_step_bams = step;
    } else {
        if (s.forward > 0.0f) force = p->slow_down;
        else if (s.forward < 0.0f) force = -p->slow_down;
    }

    /* Ordinary static support, with no pending external force or special
     * surface flags. The source clamp uses pre-turn force/post-turn speed. */
    SA1PC2004GroundAccelInput clamp_input = {
        .force_after_resistance = {resistance + g_forward, normal_force,
                                  side_resistance + g_side},
        .input_force = force,
        .speed_after_turn = {s.forward, s.normal, s.side},
        .up = frame->up.y, .friction_multiplier = 1.0f,
        .analog_active = analog.active
    };
    SA1PC2004GroundAccelResult acceleration;
    SA1PC2004LocalVector integrated;
    sa1_pc2004_ground_accel_clamp(&clamp_input, p, &acceleration);
    sa1_pc2004_pgetspeed_local(clamp_input.speed_after_turn,
        acceleration.add_velocity, analog.active, frame->up.y, 0, p, &integrated);
    s.forward = clampf(integrated.forward, -p->lim_h_spd, p->lim_h_spd);
    s.side = clampf(integrated.side, -p->lim_h_spd, p->lim_h_spd);
    s.normal = clampf(integrated.normal, -p->lim_v_spd, p->lim_v_spd);
    player->animation_accel_residual = acceleration.animation_residual;
    player->animation_accel_residual_valid = true;
    player->vel = surface_to_world(frame, s);

    if (dbg) {
        dbg->surface_force_forward_scale = forward_scale;
        dbg->surface_force_side_scale = side_scale;
        dbg->normal_unstick_bias = unstick_bias;
    }

    if (dbg) {
        dbg->forward_speed_after = s.forward;
        dbg->side_speed_after = s.side;
        dbg->tangent_gravity_forward = g_forward;
        dbg->tangent_gravity_side = g_side;
    }
}

static void surface_ball_motion(SA1PlayerState *player,
                                const SA1Input *input,
                                float camera_yaw,
                                SA1SurfaceFrame *frame,
                                SA1SteamSurfaceDebug *dbg) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1SADXFilteredAnalog analog =
        sa1_sadx_filter_logical_analog(input->move_x, input->move_z);
    SurfaceSpeed s = world_to_surface(frame, player->vel);
    if (dbg) dbg->forward_speed_before = s.forward;

    /* Retail action 4/5 begin with PGetRotation, so steering changes the local
     * frame before PGetBreak/PGetInertia calculate AddVelocity. */
    if (analog.active) {
        SA1Vec3 desired = desired_surface_direction(frame, camera_yaw, &analog);
        float delta = signed_angle_on_plane(frame->forward, desired, frame->up);
        unsigned step = general_turn_step(radians_to_bams_abs(delta));
        rotate_frame_toward(frame, &s, desired, step, SA1_PC2004_TURN_GENERAL,player->grounded);
        if (dbg) dbg->turn_step_bams = step;
    }

    /* PGetBreak/PGetInertia start from Gravity*weight plus the motionwk2
     * collision-force vector, already transformed into player coordinates.
     * The synthetic Movement Lab has no dynamic object producing that opaque
     * collision-force buffer, so this is the exact zero-extra-collision-force
     * specialization: ordinary world gravity projected into the support frame. */
    SA1Vec3 gravity_world = {0.0f, -p->weight, 0.0f};
    SA1PC2004LocalVector speed_local = {s.forward, s.normal, s.side};
    SA1PC2004LocalVector force_local = {
        sa1_vec3_dot(gravity_world, frame->forward),
        sa1_vec3_dot(gravity_world, frame->up),
        sa1_vec3_dot(gravity_world, frame->right)
    };
    SA1PC2004MotionContext context = {
        1, /* grounded status low bits */
        /* 0x496F00 sets status 0x500 on entry; ordinary release retains
         * its 0x100 ball bit. Model only these existing grounded actions,
         * not persistent status semantics for unsupported retail actions. */
        player->action_state == SA1_ACTION_SPINDASH_CHARGE ||
            player->action_state == SA1_ACTION_ROLL,
        0, /* status 0x4000 bypass */
        frame->up.y,
        1.0f /* PInitialize default motionwk2+0x3C */
    };
    SA1PC2004MotionDebug motion_debug;
    SA1PC2004LocalVector add;

    if (player->action_state == SA1_ACTION_SPINDASH_CHARGE) {
        add = sa1_pc2004_pgetbreak_add_velocity(
            speed_local, force_local, &context, p, &motion_debug);
        if (dbg) dbg->used_pc2004_pgetbreak = 1;
    } else {
        add = sa1_pc2004_pgetinertia_add_velocity(
            speed_local, force_local, &context, p, &motion_debug);
        if (dbg) {
            dbg->used_pc2004_pgetinertia = 1;
            /* PGetInertia has an additional grounded pending-collision impulse
             * consumer through playerwk+0x64. Static Movement-Lab geometry
             * produces none, so the zero-impulse specialization is explicit. */
            dbg->pc2004_pending_collision_impulse_omitted = 1;
        }
    }

    SA1PC2004LocalVector integrated;
    sa1_pc2004_pgetspeed_local(speed_local,add,analog.active,
        frame->up.y,0,p,&integrated);
    s.forward=integrated.forward;s.normal=integrated.normal;s.side=integrated.side;
    s.forward = clampf(s.forward, -p->lim_h_spd, p->lim_h_spd);
    s.side = clampf(s.side, -p->lim_h_spd, p->lim_h_spd);
    player->vel = surface_to_world(frame, s);

    if (dbg) {
        dbg->forward_speed_after = s.forward;
        dbg->side_speed_after = s.side;
        dbg->tangent_gravity_forward = force_local.forward;
        dbg->tangent_gravity_side = force_local.side;
        dbg->pc2004_local_normal_force = force_local.normal;
        dbg->pc2004_add_forward = add.forward;
        dbg->pc2004_add_side = add.side;
    }
}
static int handle_surface_ball_state(SA1PlayerState *player,
                                     const SA1Input *input,
                                     float camera_yaw,
                                     SA1SurfaceFrame *frame,
                                     SA1SteamSurfaceDebug *dbg) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SurfaceSpeed s = world_to_surface(frame, player->vel);
    int requested = input->action_pressed || input->spindash_shortcut_pressed;
    int held = input->action_held || input->spindash_shortcut_held;

    if (player->action_state == SA1_ACTION_SPINDASH_CHARGE) {
        /* Sonic_Act1 transitions precede the ball timer and movement dispatch.
         * Dispatch the new action, including inertia on the release tick. */
        if (sa1_sadx_spindash_timeout_reached(player->spindash_ticks)) {
            player->spindash_ticks = 0;
            player->action_state = SA1_ACTION_GROUNDED;
            player->pc2004_action_state_valid = false;
            player->ball_form = false;
            return 0;
        } else if (held) {
            player->spindash_charge =
                sa1_sadx_spindash_charge_held_tick(player->spindash_charge);
        } else {
            s = world_to_surface(frame, player->vel);
            s.forward = player->spindash_charge;
            s.normal = 0.0f;
            player->vel = surface_to_world(frame, s);
            player->action_state = SA1_ACTION_ROLL;
            player->pc2004_action_state = 5;
            player->pc2004_action_state_valid = true;
            player->ball_form = true;
            player->spindash_charge = 0.0f;
            player->spindash_ticks = 0;
        }
        player->spindash_ticks++;
        surface_ball_motion(player, input, camera_yaw, frame, dbg);
        return 1;
    }

    if (player->action_state == SA1_ACTION_ROLL) {
        /* Same pre-motion signed threshold as flat action 5 (0x4930D0).
         * Surface forward is the adapter's local Speed.x specialization. */
        if (s.forward < p->run_speed || requested) {
            player->action_state = SA1_ACTION_GROUNDED;
            player->pc2004_action_state = s.forward <= 0.0f ? 1 : 2;
            player->pc2004_action_state_valid = true;
            player->ball_form = false;
            return 0;
        }
        surface_ball_motion(player, input, camera_yaw, frame, dbg);
        player->spindash_ticks++;
        return 1;
    }

    if (requested) {
        player->spindash_charge =
            sa1_sadx_spindash_initial_charge(s.forward);
        player->spindash_ticks = 1;
        player->action_state = SA1_ACTION_SPINDASH_CHARGE;
        player->pc2004_action_state = 4;
        player->pc2004_action_state_valid = true;
        player->ball_form = true;
        surface_ball_motion(player, input, camera_yaw, frame, dbg);
        return 1;
    }
    return 0;
}

static int hurt_velocity(SA1PlayerState*p,const SA1Input*input,const SA1PhysicsWorld*w,SA1SurfaceFrame*f){
    const SA1SteamPlayerParameters*parameters=&SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SurfaceSpeed speed=world_to_surface(f,p->vel);
    double magnitude=(double)speed.forward*speed.forward+(double)speed.side*speed.side+(double)speed.normal*speed.normal;
    if((p->grounded||p->pc2004_action_state==86)&&((p->pc2004_player_flags4&0x8000)||magnitude<=(double)parameters->dash_speed*parameters->dash_speed)){
        SA1Vec3 foot=sa1_vec3_sub(p->pos,sa1_vec3_scale(f->up,SA1_SUPPORT_OFFSET_CANDIDATE));
        if(!sa1_pc2004_task_relax_up(f,w->homing?&w->homing->math:NULL))return 0;
        p->pos=sa1_vec3_add(foot,sa1_vec3_scale(f->up,SA1_SUPPORT_OFFSET_CANDIDATE));
        speed=world_to_surface(f,p->vel);
    }
    SA1PC2004LocalVector local={speed.forward,speed.normal,speed.side};
    SA1Vec3 gravity={0,-parameters->weight,0};
    SA1PC2004LocalVector force={sa1_vec3_dot(gravity,f->forward),sa1_vec3_dot(gravity,f->up),sa1_vec3_dot(gravity,f->right)};
    SA1PC2004MotionContext context={p->grounded,0,(p->pc2004_status_flags&0x4000)!=0,f->up.y,1.f};
    SA1PC2004LocalVector add=sa1_pc2004_pgetbreak_add_velocity(local,force,&context,parameters,NULL),next;
    sa1_pc2004_pgetspeed_local(local,add,sa1_sadx_filter_logical_analog(input->move_x,input->move_z).active,f->up.y,0,parameters,&next);
    p->vel=surface_to_world(f,(SurfaceSpeed){next.forward,next.side,next.normal});
    p->animation_accel_residual_valid=false;return 1;
}

/*492300 is a complete source velocity update at EACH invocation, including
 * initialization. TRACK bypasses gravity/acceleration; other outcomes call
 * the existing44C270+443F50 reconstruction once. Position is integrated later.*/
static int homing_velocity(SA1PlayerState*player,const SA1Input*input,float camera_yaw,
    SA1SurfaceFrame*frame,SA1SteamSurfaceDebug*dbg,const SA1PC2004HomingServices*h){
    uint16_t raw_yaw;if(!h||!sa1_pc2004_task_yaw(frame,&raw_yaw))goto fail;
    SurfaceSpeed speed=world_to_surface(frame,player->vel);
    SA1PC2004AirAttack state={.action=14,.timer=player->pc2004_air_attack_timer,.age=player->pc2004_air_attack_age};
    SA1PC2004HomingInput in={.list=h->list,
        .player_task_origin=sa1_vec3_sub(player->pos,sa1_vec3_scale(frame->up,SA1_SUPPORT_OFFSET_CANDIDATE)),
        .local_speed={speed.forward,speed.normal,-speed.side},.task_yaw=raw_yaw,.partner_id=h->partner_id,
        .special_player=h->special_player,.resolve=h->resolve,.resolve_context=h->resolve_context,.math=h->math};
    SA1PC2004HomingResult result;if(!sa1_pc2004_homing_step(&state,&in,&result))goto fail;
    player->pc2004_homing_calls++;player->pc2004_air_attack_timer=result.next.timer;
    if(result.mode==SA1_PC2004_HOMING_TRACK){
        if(!sa1_pc2004_task_turn(frame,raw_yaw,result.task_yaw,&h->math))goto fail;
        player->vel=result.world_velocity;player->pc2004_homing_target=result.selected_id;player->pc2004_homing_tracks++;
    }else{
        player->vel=surface_to_world(frame,(SurfaceSpeed){result.local_speed.forward,-result.local_speed.side,result.local_speed.normal});
        surface_running(player,input,camera_yaw,frame,dbg);
    }
    return 1;
fail:player->pc2004_homing_failed=true;return 0;
}
static int homing_initialize(SA1PlayerState*p,const SA1Input*input,float camera_yaw,
    SA1SurfaceFrame*frame,SA1SteamSurfaceDebug*dbg,const SA1PC2004HomingServices*h){
    if(!h||!h->list){p->pc2004_homing_failed=true;return 0;}
    if(!h->list->count&&!h->partner_id){
        SurfaceSpeed speed=world_to_surface(frame,p->vel);speed.forward=h->special_player?10.f:5.f;
        p->vel=surface_to_world(frame,speed);p->pc2004_air_attack_timer=1;return 1;
    }
    if(!homing_velocity(p,input,camera_yaw,frame,dbg,h))return 0;
    /*494B80's post-call nonzero-timer write is ALWAYS5, including special
     * player. The10 branch applies only to the truly empty-list initializer.*/
    if(p->pc2004_air_attack_timer!=0){SurfaceSpeed speed=world_to_surface(frame,p->vel);speed.forward=5.f;p->vel=surface_to_world(frame,speed);}
    return 1;
}

void sa1_physics_step_steam_sonic_surface(
    SA1PlayerState *player,
    const SA1Input *input,
    float camera_yaw,
    const SA1PhysicsWorld *world,
    const SA1PhysicsConstants *compat_constants,
    SA1SteamSurfaceDebug *debug_out) {
    (void)compat_constants;
    SA1SteamSurfaceDebug local_debug;
    SA1SteamSurfaceDebug *dbg = debug_out ? debug_out : &local_debug;
    memset(dbg, 0, sizeof(*dbg));
    dbg->used_general_contact = 1;
    int just_jumped = 0;
    player->pc2004_homing_calls=player->pc2004_homing_tracks=0;
    player->pc2004_homing_failed=false;player->pc2004_homing_target=0;
    player->animation_input_active = sa1_sadx_filter_logical_analog(
        input->move_x, input->move_z).active != 0;
    player->animation_input_valid = true;
    player->animation_landing_valid = false;
    player->animation_forward_speed_valid = false;
    player->surface_frame_active = true;

    SA1SurfaceFrame frame = player_frame(player);
    if (!frame.valid) {
        frame = sa1_surface_frame_make((SA1Vec3){0.0f,1.0f,0.0f},
                                       horizontal_from_yaw(player->yaw));
    }

    SA1PC2004CollisionWork collision_work;
    sa1_pc2004_collision_work_init(&collision_work,
                                   &SA1_STEAM_SONIC_PARAMETER_EVIDENCE,
                                   player->pos, player->vel, 0, 0);
    dbg->pc2004_collision_radius = collision_work.radius;
    dbg->pc2004_collision_work_reconstructed = 1;
    dbg->pc2004_retail_polygon_solver_pending = 1;

    int dead_action=player->pc2004_action_state_valid&&player->pc2004_action_state==86;
    int hurt_action=player->pc2004_action_state_valid&&(player->pc2004_action_state==16||dead_action);
    /*49BDB4 requests87 from incoming ground flags before PSetPosition, even
     * on the first death frame whose positive normal velocity departs ground.*/
    if(dead_action&&player->grounded){
        player->animation_script_serial++;player->animation_script_request=87;player->animation_script_valid=true;
    }
    int skip_attack_hold=0;
    if(!player->grounded&&player->pc2004_action_state_valid&&
       (player->pc2004_action_state==8||player->pc2004_action_state==14)){
        const SA1PC2004HomingServices*h=world->homing;
        SA1PC2004AirAttack current={player->pc2004_action_state,
            player->ball_form?0x500u:0u,
            player->animation_airborne_request_valid?player->animation_airborne_request:14,
            player->pc2004_air_attack_timer,player->pc2004_air_attack_age};
        SA1PC2004AirAttackSelection selection=h?
            sa1_pc2004_air_attack_select(current,input->jump_pressed,input->action_pressed,h->first_hit_category):
            sa1_pc2004_air_attack_select_no_target(current,input->jump_pressed,input->action_pressed,-1,0);
        player->pc2004_action_state=selection.next.action;
        player->pc2004_air_attack_timer=selection.next.timer;
        player->pc2004_air_attack_age=selection.next.age;
        player->ball_form=(selection.next.status&0x100u)!=0;
        player->animation_airborne_request=selection.next.animation;
        player->animation_airborne_request_valid=true;skip_attack_hold=selection.skip_jump_hold;
        if(selection.write_forward){
            SurfaceSpeed speed=world_to_surface(&frame,player->vel);speed.forward=selection.forward;
            player->vel=surface_to_world(&frame,speed);
        }
        if(selection.entered&&h&&!homing_initialize(player,input,camera_yaw,&frame,dbg,h))return;
    }
    int source_air_attack=player->pc2004_action_state_valid&&player->pc2004_action_state==14;
    if(hurt_action){
        if(!hurt_velocity(player,input,world,&frame)){player->pc2004_homing_failed=true;return;}
    }else if(source_air_attack&&world->homing){
        if(!homing_velocity(player,input,camera_yaw,&frame,dbg,world->homing))return;
    }else if(source_air_attack){
        SurfaceSpeed speed=world_to_surface(&frame,player->vel);
        SA1PC2004LocalVector local={speed.forward,speed.normal,speed.side};
        SA1PC2004AirAttack clock={.action=14,.timer=player->pc2004_air_attack_timer};
        if(sa1_pc2004_air_attack_decay(&clock,&local)){
            player->pc2004_air_attack_timer=clock.timer;
            player->vel=surface_to_world(&frame,(SurfaceSpeed){local.forward,local.side,local.normal});
            surface_running(player,input,camera_yaw,&frame,dbg);
        }
    }else{
    int source_spring=player->pc2004_action_state_valid &&
        (player->pc2004_action_state==9 || player->pc2004_action_state==10);
    if(source_spring) {
        float normal_speed=sa1_vec3_dot(player->vel,frame.up);
        if(player->pc2004_action_state==10 &&
           normal_speed>=SA1_STEAM_SONIC_PARAMETER_EVIDENCE.nocon_speed)
            sa1_steam_sonic_noinput_step_surface(player,input,&frame);
        else sa1_steam_sonic_air_step_surface(player,input,camera_yaw,&frame,NULL);
        /* The incoming low ground bits participate in the launch movement;
         * rising motion then reaches airborne contact resolution. */
        player->grounded=false;
        player->animation_accel_residual=0;
        player->animation_accel_residual_valid=true;
    } else {
    if (player->grounded) {
        /* Retail Sonic action handlers run PGetRotation/acceleration/speed
         * before PSetPosition -> PResetPosition.  Keep the prior frame's
         * support while computing movement. Only a freshly reset/unresolved
         * actor performs an initial acquisition query here. */
        if (player->surface_id < 0) {
            if (!refresh_support(player, world, &frame, dbg, &collision_work, SUPPORT_QUERY_INITIAL))
                detach_player(player, dbg);
        }

        if (player->grounded) {
            /* Sonic_Act1 selects jump before ground movement, including
             * roll termination, but charging has no jump transition. */
            if (player->action_state != SA1_ACTION_SPINDASH_CHARGE && input->jump_pressed) {
                begin_surface_jump(player, &frame);
                just_jumped = 1;
            } else {
                int ball_handled = handle_surface_ball_state(player, input,
                                    camera_yaw, &frame, dbg);
                if (!ball_handled) {
                    /* Action 2 evaluates 0x494FF0 before PGetRotation /
                     * acceleration / speed.  Snapshot the local forward
                     * component in the incoming support frame. */
                    float selector_forward =
                        sa1_vec3_dot(player->vel, frame.forward);
                    int selector_input_active = player->animation_input_active;
                    surface_running(player, input, camera_yaw, &frame, dbg);
                    if (player->pc2004_action_state_valid) {
                        if (player->pc2004_action_state == 2 &&
                            !selector_input_active &&
                            selector_forward == 0.0f) {
                            player->pc2004_action_state = 1;
                        } else if (player->pc2004_action_state == 1 &&
                                   selector_input_active) {
                            player->pc2004_action_state = 2;
                        }
                    }
                }
            }
        }
    }

    if (!player->grounded) {
        /* PC2004 PGetAccelerationAir transforms gravity through Sonic's full
         * task rotation. Retain the launch/support frame for local Speed.y,
         * anisotropic drag, and general-turn steering instead of collapsing
         * inclined launches into the flat world-Y specialization. */
        if (!just_jumped && !skip_attack_hold && (!player->pc2004_action_state_valid ||
                             player->pc2004_action_state == 8))
            (void)sa1_steam_sonic_apply_jump_hold_surface(
                player, &frame, input->jump_held);
        sa1_steam_sonic_air_step_surface(player, input, camera_yaw, &frame, NULL);
    }
    }
    }
    /* Commit steering even if the following support query misses a ledge.
     * The next airborne tick must retain this orientation, not the old frame. */
    sync_player_frame(player, &frame, NULL);

    /* Source49AA48 strength uses the selected action/ball, never the action
     * rewritten by the following terrain landing/departure resolution. */
    sa1_player_capture_ccl_state(player);
    player->pos = sa1_vec3_add(player->pos, player->vel);

    if (player->grounded) {
        /* 2004 action ordering reaches collision only after movement.  The old
         * 0.3/1.16 forced-fall rule came from a separate public SA1-style
         * reimplementation; exact 2004 PSetPosition/PResetPosition do not
         * directly reference the 1.16 constant, so that heuristic is no longer
         * allowed to force a detach in the primary PC profile. */
        dbg->legacy_low_speed_fall_disabled = 1;
        if (!refresh_support(player, world, &frame, dbg, &collision_work, SUPPORT_QUERY_POST_MOTION))
            detach_player(player, dbg);
    } else {
        /* Landing probe uses world down until airborne orientation parity is
         * reconstructed. This preserves 0.0.8.5 landing behavior on floors. */
        SA1Contact landing = sa1_world_query_contact(
            world, player->pos, (SA1Vec3){0.0f,-1.0f,0.0f},
            SA1_SUPPORT_QUERY_RANGE_CANDIDATE);
        if (landing.valid && player->vel.y <= 0.0f && landing.normal.y > 0.45f &&
            landing.signed_distance >= 0.0f &&
            landing.signed_distance <= SA1_SUPPORT_QUERY_RANGE_CANDIDATE) {
            if (player->action_state == SA1_ACTION_AIRBORNE) {
                /* Ordinary static-support equivalent of retained motionwk Y.
                 * Do not classify from the post-projection local normal zero. */
                player->animation_landing_world_y = player->vel.y;
                player->animation_landing_air_action =
                    (player->pc2004_action_state_valid &&
                     (player->pc2004_action_state == 8 ||
                      player->pc2004_action_state == 12)) ?
                    player->pc2004_action_state : (player->ball_form ? 8 : 12);
                player->animation_landing_valid = isfinite(player->vel.y);
            }
            frame = sa1_surface_frame_make(landing.normal,
                                           horizontal_from_yaw(player->yaw));
            player->pos = sa1_vec3_add(landing.point,
                        sa1_vec3_scale(frame.up, SA1_SUPPORT_OFFSET_CANDIDATE));
            SurfaceSpeed s = world_to_surface(&frame, player->vel);
            s.normal = 0.0f;
            player->vel = surface_to_world(&frame, s);
            player->grounded = true;
            player->jump_ticks = 0;
            if (player->action_state == SA1_ACTION_AIRBORNE) {
                player->action_state = SA1_ACTION_GROUNDED;
                player->ball_form = false;
            }
            if(!hurt_action){
                player->pc2004_action_state = (player->animation_input_active || s.forward != 0.0f) ? 2 : 1;
                player->pc2004_action_state_valid = true;
            }
            player->animation_airborne_request_valid = false;
            sync_player_frame(player, &frame, &landing);
            sa1_pc2004_collision_work_mirror_static_contact(&collision_work, &landing, 0);
        }
    }

    dbg->pc2004_collision_find_count = collision_work.find_count;

    if(hurt_action)player->action_state=dead_action?SA1_ACTION_DEAD:SA1_ACTION_HURT;
    else if (!player->grounded)
        player->action_state = SA1_ACTION_AIRBORNE;
    else if (player->action_state != SA1_ACTION_SPINDASH_CHARGE &&
             player->action_state != SA1_ACTION_ROLL)
        player->action_state = SA1_ACTION_GROUNDED;

    if (player->pos.y < world->reset_min_y ||
        player->pos.x < world->reset_min_x || player->pos.x > world->reset_max_x ||
        player->pos.z < world->reset_min_z || player->pos.z > world->reset_max_z) {
        sa1_player_reset(player, world);
        return;
    }

    /* Match the local coordinate system used by surface movement. Horizontal
     * yaw projection loses vertical motion on ramps/walls and under-paces
     * animation. Capture after support/landing has finalized the frame. */
    player->animation_forward_speed =
        sa1_vec3_dot(player->vel, player->surface_forward);
    player->animation_forward_speed_valid =
        isfinite(player->animation_forward_speed);

    float speed = sa1_surface_tangent_speed(player);
    if (speed > player->peak_speed) player->peak_speed = speed;
    if (player->pos.y > player->peak_y) player->peak_y = player->pos.y;
    player->sim_frame++;
}
