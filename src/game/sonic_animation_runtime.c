#include "sonic_animation_runtime.h"

#include <string.h>

static const SA3DSSonicAnimationBinding kBindings[] = {
    { 0,  1, "Stand"},
    { 9,  3, "Walk1"},
    {10,  4, "Walk2"},
    {11,  5, "Walk3"},
    {12,  6, "Run1"},
    {13, 18, "Run2"},
    {14, 14, "Jump/Spin"},
    {15, 14, "Roll"},
    {16,  8, "Spring"},
    {17,  9, "Spring->Fall"},
    {18, 10, "Fall"},
    {19, 11, "Land"},
    {20,  7, "Brake"},
    {29, 22, "Uncurl"},
    {30, 20, "Turn Around"},
    {31, 19, "Skidding"},
    {32, 21, "Spin Ball"},
};

size_t sa3ds_sonic_animation_binding_count(void) {
    return sizeof(kBindings) / sizeof(kBindings[0]);
}

int sa3ds_sonic_animation_binding_at(size_t index, SA3DSSonicAnimationBinding *out) {
    if (!out || index >= sa3ds_sonic_animation_binding_count()) return 0;
    *out = kBindings[index];
    return 1;
}

int sa3ds_sonic_animation_binding(uint16_t runtime_id,
                                  SA3DSSonicAnimationBinding *out) {
    for (unsigned i = 0; i < sizeof(kBindings) / sizeof(kBindings[0]); ++i) {
        if (kBindings[i].runtime_id == runtime_id) {
            if (out) *out = kBindings[i];
            return 1;
        }
    }
    if (out) memset(out, 0, sizeof(*out));
    return 0;
}

static uint16_t choose_runtime_id(const SA1PlayerState *player,
                                  const SA3DSSonicAnimationObservation *o,
                                  int previous_grounded) {
    /* One-tick landing presentation hook. Exact retail duration/transition is
     * not claimed; this exists so hardware testing can expose the real gap. */
    if (player->grounded && !previous_grounded)
        return 19;

    switch (o->intent) {
    case SA3DS_SONIC_ANIM_GROUNDED_IDLE:
        return 0;
    case SA3DS_SONIC_ANIM_GROUNDED_BELOW_JOG:
        return 9;
    case SA3DS_SONIC_ANIM_GROUNDED_JOG_TO_RUN:
        return 10;
    case SA3DS_SONIC_ANIM_GROUNDED_RUN_PLUS:
        return 12;
    case SA3DS_SONIC_ANIM_AIRBORNE_UPRIGHT:
        return 18;
    case SA3DS_SONIC_ANIM_AIRBORNE_BALL:
        return 14;
    case SA3DS_SONIC_ANIM_SPINDASH_CHARGE:
        return 32;
    case SA3DS_SONIC_ANIM_ROLL:
        return 15;
    default:
        return 0;
    }
}

SA3DSSonicAnimationSelection sa3ds_sonic_animation_select(
    SA3DSSonicAnimationRuntime *runtime,
    const SA1PlayerState *player,
    const SA3DSSonicAnimationObservation *observation) {
    SA3DSSonicAnimationSelection out;
    memset(&out, 0, sizeof(out));
    out.selector_is_provisional = 1;
    if (!runtime || !player || !observation) return out;

    /* A landing transition lasts one *physics tick*, not one call. A second
     * renderer/HUD query in the same tick must retain the landing pose. */
    if (runtime->initialized) {
        if (player->sim_frame == runtime->last_sim_frame)
            return runtime->cached_selection;
        if (player->sim_frame < runtime->last_sim_frame)
            *runtime = (SA3DSSonicAnimationRuntime){0};
    }

    int previous_grounded = runtime->initialized ? runtime->previous_grounded
                                                  : player->grounded;
    uint16_t id = choose_runtime_id(player, observation, previous_grounded);
    if (!sa3ds_sonic_animation_binding(id, &out.binding))
        (void)sa3ds_sonic_animation_binding(0, &out.binding);

    if (!runtime->initialized || runtime->previous_runtime_id != out.binding.runtime_id) {
        runtime->selection_entry_frame = player->sim_frame;
        runtime->previous_runtime_id = out.binding.runtime_id;
    }
    runtime->initialized = 1;
    runtime->previous_grounded = player->grounded ? 1 : 0;

    out.ticks_in_selection = player->sim_frame >= runtime->selection_entry_frame
        ? player->sim_frame - runtime->selection_entry_frame : 0;

    /* 0E's 0.5 source-frame/tick playback is retained deliberately as a
     * hardware-test baseline. It is NOT yet evidence for retail playback rate. */
    out.sample_frame = (float)out.ticks_in_selection * 0.5f;
    runtime->last_sim_frame = player->sim_frame;
    runtime->cached_selection = out;
    return out;
}
