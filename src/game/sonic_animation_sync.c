#include "sonic_animation_sync.h"

#include <math.h>

#include "../sa1/physics/surface_frame.h"

#define SA3DS_ANIM_SPEED_EPSILON 0.00001f
#define SA3DS_ANIM_AIR_EPSILON 0.00001f

static SA3DSSonicAirPhase classify_air_phase(const SA1PlayerState *player) {
    if (player->grounded) return SA3DS_SONIC_AIR_NONE;
    if (player->vel.y > SA3DS_ANIM_AIR_EPSILON) return SA3DS_SONIC_AIR_RISING;
    if (player->vel.y < -SA3DS_ANIM_AIR_EPSILON) return SA3DS_SONIC_AIR_FALLING;
    return SA3DS_SONIC_AIR_APEX;
}

static SA3DSSonicAnimationIntent classify_intent(
    const SA1PlayerState *player,
    const SA1SteamPlayerParameters *parameters,
    float *tangent_speed_out) {
    float tangent_speed = 0.0f;

    if (player->action_state == SA1_ACTION_SPINDASH_CHARGE)
        return SA3DS_SONIC_ANIM_SPINDASH_CHARGE;

    if (player->action_state == SA1_ACTION_ROLL)
        return SA3DS_SONIC_ANIM_ROLL;

    if (!player->grounded || player->action_state == SA1_ACTION_AIRBORNE)
        return player->ball_form ? SA3DS_SONIC_ANIM_AIRBORNE_BALL
                                 : SA3DS_SONIC_ANIM_AIRBORNE_UPRIGHT;

    tangent_speed = sa1_surface_tangent_speed(player);
    if (tangent_speed_out) *tangent_speed_out = tangent_speed;

    if (tangent_speed <= SA3DS_ANIM_SPEED_EPSILON)
        return SA3DS_SONIC_ANIM_GROUNDED_IDLE;

    if (tangent_speed < parameters->jog_speed)
        return SA3DS_SONIC_ANIM_GROUNDED_BELOW_JOG;

    if (tangent_speed < parameters->run_speed)
        return SA3DS_SONIC_ANIM_GROUNDED_JOG_TO_RUN;

    return SA3DS_SONIC_ANIM_GROUNDED_RUN_PLUS;
}

SA3DSSonicAnimationObservation sa3ds_sonic_animation_observe(
    SA3DSSonicAnimationSync *sync,
    const SA1PlayerState *player,
    const SA1SteamPlayerParameters *parameters) {
    SA3DSSonicAnimationObservation out = {0};
    if (!sync || !player || !parameters) return out;

    /* Physics completes once per sim_frame. Rendering, diagnostics, and
     * paused-frame inspection may query the observer repeatedly; those
     * queries must not consume a new transition or change the snapshot.
     * A trace rewind invalidates the old intent timeline. */
    if (sync->initialized) {
        if (player->sim_frame == sync->last_physics_frame)
            return sync->cached_observation;
        if (player->sim_frame < sync->last_physics_frame)
            *sync = (SA3DSSonicAnimationSync){0};
    }

    float tangent_speed = 0.0f;
    SA3DSSonicAnimationIntent intent =
        classify_intent(player, parameters, &tangent_speed);

    if (!sync->initialized || sync->intent != intent) {
        sync->initialized = true;
        sync->intent = intent;
        sync->intent_entry_frame = player->sim_frame;
    }

    out.intent = intent;
    out.air_phase = classify_air_phase(player);
    out.tangent_speed = tangent_speed;
    out.physics_frame = player->sim_frame;
    out.intent_entry_frame = sync->intent_entry_frame;
    out.ticks_in_intent = player->sim_frame >= sync->intent_entry_frame
        ? player->sim_frame - sync->intent_entry_frame : 0;
    out.ball_form = player->ball_form;
    sync->last_physics_frame = player->sim_frame;
    sync->cached_observation = out;
    return out;
}

const char *sa3ds_sonic_animation_intent_name(SA3DSSonicAnimationIntent intent) {
    switch (intent) {
    case SA3DS_SONIC_ANIM_GROUNDED_IDLE: return "grounded_idle";
    case SA3DS_SONIC_ANIM_GROUNDED_BELOW_JOG: return "grounded_below_jog";
    case SA3DS_SONIC_ANIM_GROUNDED_JOG_TO_RUN: return "grounded_jog_to_run";
    case SA3DS_SONIC_ANIM_GROUNDED_RUN_PLUS: return "grounded_run_plus";
    case SA3DS_SONIC_ANIM_AIRBORNE_UPRIGHT: return "airborne_upright";
    case SA3DS_SONIC_ANIM_AIRBORNE_BALL: return "airborne_ball";
    case SA3DS_SONIC_ANIM_SPINDASH_CHARGE: return "spindash_charge";
    case SA3DS_SONIC_ANIM_ROLL: return "roll";
    default: return "unknown";
    }
}

const char *sa3ds_sonic_air_phase_name(SA3DSSonicAirPhase phase) {
    switch (phase) {
    case SA3DS_SONIC_AIR_NONE: return "none";
    case SA3DS_SONIC_AIR_RISING: return "rising";
    case SA3DS_SONIC_AIR_APEX: return "apex";
    case SA3DS_SONIC_AIR_FALLING: return "falling";
    default: return "unknown";
    }
}
