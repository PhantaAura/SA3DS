#ifndef SA3DS_GAME_SONIC_ANIMATION_RUNTIME_H
#define SA3DS_GAME_SONIC_ANIMATION_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

#include "sonic_animation_sync.h"

typedef struct {
    uint16_t runtime_id;
    uint16_t action_slot;
    uint8_t instance;
    int8_t property;
    uint16_t next_runtime_id;
    float transition_speed;
    float animation_speed;
    uint16_t source_root_node;
    uint16_t source_frame_count;
    const char *name;
} SA3DSSonicAnimationBinding;

typedef struct {
    /* CharObj2.Speed.x in PC2004: signed player-local forward speed. */
    float forward_speed;
    /* CharObj2.SpindashSpeed: state routines prepare this shared scalar. */
    float speed_scalar;
} SA3DSSonicAnimationSpeedInputs;

typedef struct {
    SA3DSSonicAnimationBinding binding;
    float sample_frame;
    uint64_t ticks_in_selection;
    int playback_rate_is_source_backed;
    int selector_is_provisional;
    int transition_active;
    uint16_t transition_from_action_slot;
    float transition_from_frame;
    float transition_weight;
} SA3DSSonicAnimationSelection;

typedef struct {
    int initialized;
    int previous_grounded;
    SA1ActionState previous_action_state;
    /* Persistent retail request; NextAnim writes both request and index. */
    uint16_t requested_runtime_id;
    SA3DSSonicAnimationBinding active_binding;
    uint64_t selection_entry_frame;
    uint64_t last_sim_frame;
    float accumulated_sample_frame;
    float speed_scalar;
    uint8_t speed_scalar_valid;
    uint8_t transition_active;
    uint16_t transition_from_action_slot;
    float transition_from_frame;
    float transition_progress;
    SA3DSSonicAnimationSelection cached_selection;
} SA3DSSonicAnimationRuntime;

/* Exact recovered dense-runtime -> CHRMODELS ACTION map for the IDs used by
 * the current White Zone runtime. Returns 0 for an unknown/unbound ID. */
size_t sa3ds_sonic_animation_binding_count(void);
int sa3ds_sonic_animation_binding_at(size_t index, SA3DSSonicAnimationBinding *out);

int sa3ds_sonic_animation_binding(uint16_t runtime_id,
                                  SA3DSSonicAnimationBinding *out);

/* Exact PC2004 PlayAnimation (0x0044A800) property 9/10/11 arithmetic.
 * Property 9 uses forward_speed * AnimationSpeed. Properties 10 and 11 use
 * the stateful CharObj2.SpindashSpeed scalar. Sonic's ground-animation
 * selector (0x00495CD0) prepares forward_speed + 20 * acceleration residual;
 * other state transitions can assign constants, including jump's 5.0 at
 * 0x00495E60. Property 10 multiplies the scalar by AnimationSpeed; property 11
 * consumes it directly. */
int sa3ds_sonic_animation_frame_step(
    const SA3DSSonicAnimationBinding *binding,
    const SA3DSSonicAnimationSpeedInputs *inputs,
    float *out_step);

/*
 * Presentation-only selector. Dense-ID -> ACTION mappings, the five ground
 * speed bands, and PlayAnimation transition mechanics are source-backed.
 * The broader intent bridge remains provisional where Sonic action-state
 * inputs (including soft-versus-hard landing) are not yet represented.
 * Selection is stable across repeated queries of one completed sim_frame;
 * rewind clears the prior transition history. Explicit reset is required
 * when switching zones/players at the same sim_frame.
 * This routine must never mutate SA1PlayerState.
 */
SA3DSSonicAnimationSelection sa3ds_sonic_animation_select(
    SA3DSSonicAnimationRuntime *runtime,
    const SA1PlayerState *player,
    const SA3DSSonicAnimationObservation *observation);

#endif
