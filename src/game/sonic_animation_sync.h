#ifndef SA3DS_GAME_SONIC_ANIMATION_SYNC_H
#define SA3DS_GAME_SONIC_ANIMATION_SYNC_H

#include <stdbool.h>
#include <stdint.h>

#include "../sa1/physics/player_state.h"
#include "../sa1/physics/steam_player_parameters.h"

typedef enum {
    SA3DS_SONIC_ANIM_GROUNDED_IDLE = 0,
    SA3DS_SONIC_ANIM_GROUNDED_BELOW_JOG = 1,
    SA3DS_SONIC_ANIM_GROUNDED_JOG_TO_RUN = 2,
    SA3DS_SONIC_ANIM_GROUNDED_RUN_PLUS = 3,
    SA3DS_SONIC_ANIM_AIRBORNE_UPRIGHT = 4,
    SA3DS_SONIC_ANIM_AIRBORNE_BALL = 5,
    SA3DS_SONIC_ANIM_SPINDASH_CHARGE = 6,
    SA3DS_SONIC_ANIM_ROLL = 7
} SA3DSSonicAnimationIntent;

typedef enum {
    SA3DS_SONIC_AIR_NONE = 0,
    SA3DS_SONIC_AIR_RISING = 1,
    SA3DS_SONIC_AIR_APEX = 2,
    SA3DS_SONIC_AIR_FALLING = 3
} SA3DSSonicAirPhase;

typedef struct {
    SA3DSSonicAnimationIntent intent;
    SA3DSSonicAirPhase air_phase;
    float tangent_speed;
    uint64_t physics_frame;
    uint64_t intent_entry_frame;
    uint64_t ticks_in_intent;
    bool ball_form;
} SA3DSSonicAnimationObservation;

typedef struct {
    bool initialized;
    SA3DSSonicAnimationIntent intent;
    uint64_t intent_entry_frame;
    uint64_t last_physics_frame;
    SA3DSSonicAnimationObservation cached_observation;
} SA3DSSonicAnimationSync;

/*
 * Presentation-only bridge.
 *
 * The frozen SA1PlayerState is observed after a physics tick. This module
 * cannot write velocity, position, collision, action state, or any physics
 * parameter. The eventual real ACTION/MOTION resolver consumes the returned
 * intent; it does not feed back into physics.
 *
 * One completed physics state is observed per simulation frame; repeated
 * presentation queries of that frame return the same snapshot. A rewind in
 * sim_frame starts a new observation timeline. Reset explicitly when loading
 * a different player/zone at the same frame.
 *
 * jog_speed/run_speed are exact Sonic parameter evidence. They are used here
 * only to create physics-backed speed classes. They are NOT yet claimed as
 * exact SADX animation transition thresholds.
 */
SA3DSSonicAnimationObservation sa3ds_sonic_animation_observe(
    SA3DSSonicAnimationSync *sync,
    const SA1PlayerState *player,
    const SA1SteamPlayerParameters *parameters);

const char *sa3ds_sonic_animation_intent_name(SA3DSSonicAnimationIntent intent);
const char *sa3ds_sonic_air_phase_name(SA3DSSonicAirPhase phase);

#endif
