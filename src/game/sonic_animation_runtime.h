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
    const char *name;
} SA3DSSonicAnimationBinding;

typedef struct {
    SA3DSSonicAnimationBinding binding;
    float sample_frame;
    uint64_t ticks_in_selection;
    int playback_rate_is_source_backed;
    int selector_is_provisional;
} SA3DSSonicAnimationSelection;

typedef struct {
    int initialized;
    int previous_grounded;
    uint16_t previous_runtime_id;
    uint64_t selection_entry_frame;
    uint64_t last_sim_frame;
    SA3DSSonicAnimationSelection cached_selection;
} SA3DSSonicAnimationRuntime;

/* Exact recovered dense-runtime -> CHRMODELS ACTION map for the IDs used by
 * the current White Zone runtime. Returns 0 for an unknown/unbound ID. */
size_t sa3ds_sonic_animation_binding_count(void);
int sa3ds_sonic_animation_binding_at(size_t index, SA3DSSonicAnimationBinding *out);

int sa3ds_sonic_animation_binding(uint16_t runtime_id,
                                  SA3DSSonicAnimationBinding *out);

/*
 * Presentation-only selector. Dense-ID -> ACTION mappings are source-backed;
 * choosing which dense ID to play for a given broad physics intent remains a
 * provisional policy until the retail animation-state switch code is traced.
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
