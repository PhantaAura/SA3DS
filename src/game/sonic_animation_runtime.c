#include "sonic_animation_runtime.h"

#include <math.h>
#include <string.h>

static const SA3DSSonicAnimationBinding kBindings[] = {
    /* Exact scalar fields recovered from PC2004 InitSonicAnimData (0x7CEC90).
     * Property meanings are verified against PlayAnimation (0x44A800). */
    { 0,  1, 73,  3,  0, 0.0625f, 0.5f, "Stand"},
    { 9,  3, 73, 10,  9, 0.25f,   3.4f, "Walk1"},
    {10,  4, 73, 10, 10, 0.25f,   1.4f, "Walk2"},
    {11,  5, 73, 10, 11, 0.25f,   0.8f, "Walk3"},
    {12,  6, 73, 10, 12, 0.25f,   0.5f, "Run1"},
    {13, 18, 73,  9, 13, 0.5f,    0.5f, "Run2"},
    {14, 14, 74, 10, 14, 0.25f,   0.2f, "Jump/Spin"},
    {15, 14, 74,  9, 15, 0.25f,   0.7f, "Roll"},
    {16,  8, 73,  3, 16, 0.25f,   1.0f, "Spring"},
    {17,  9, 73,  4, 18, 0.125f,  1.0f, "Spring->Fall"},
    {18, 10, 73,  3, 18, 0.25f,   1.0f, "Fall"},
    {19, 11, 73,  4,  0, 1.0f,    1.0f, "Land"},
    {20,  7, 73,  4,  0, 0.25f,   1.0f, "Brake"},
    {29, 22, 73,  4,  1, 0.0625f, 1.0f, "Uncurl"},
    {30, 20, 73,  4,  0, 1.0f,    0.9f, "Turn Around"},
    {31, 19, 73,  3, 31, 0.0625f, 1.0f, "Skidding"},
    {32, 21,  1,  3, 32, 0.5f,    1.0f, "Spin Ball"},
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

int sa3ds_sonic_animation_frame_step(
    const SA3DSSonicAnimationBinding *binding,
    const SA3DSSonicAnimationSpeedInputs *inputs,
    float *out_step) {
    if (!binding || !out_step) return 0;
    switch (binding->property) {
    case 3: /* loop forward */
    case 4: /* advance then transition to NextAnim */
    case 5: /* advance and clamp */
    case 8: /* advance then transition with blend bookkeeping */
        *out_step = binding->animation_speed;
        return 1;
    case 6: /* loop backward */
    case 7: /* reverse and clamp */
        *out_step = -binding->animation_speed;
        return 1;
    case 9: /* loop using pfloat18: CharObj2.Speed.x */
        if (!inputs) return 0;
        *out_step = inputs->forward_speed * binding->animation_speed;
        return 1;
    case 10: /* loop using pfloat1C prepared by Sonic's selector */
        if (!inputs) return 0;
        *out_step = inputs->speed_scalar * binding->animation_speed;
        return 1;
    case 11: /* advance/transition using pfloat1C directly */
        if (!inputs) return 0;
        *out_step = inputs->speed_scalar;
        return 1;
    default:
        return 0;
    }
}

static float local_forward_speed(const SA1PlayerState *player) {
    /* Matches CharObj2.Speed.x for the verified flat/upright runtime path. */
    return player->vel.x * sinf(player->yaw) +
           player->vel.z * cosf(player->yaw);
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

    int changed_selection = !runtime->initialized ||
        runtime->previous_runtime_id != out.binding.runtime_id;
    if (changed_selection) {
        runtime->selection_entry_frame = player->sim_frame;
        runtime->previous_runtime_id = out.binding.runtime_id;
        runtime->accumulated_sample_frame = 0.0f;
    }

    /* The second retail pointer targets stateful CharObj2.SpindashSpeed.
     * Ground locomotion refreshes it every selector call. The jump transition
     * that selects action 14 instead assigns 5.0 and leaves it persistent. */
    if (out.binding.runtime_id >= 9 && out.binding.runtime_id <= 12) {
        runtime->speed_scalar = local_forward_speed(player) +
            player->animation_accel_residual * 20.0f;
        runtime->speed_scalar_valid =
            player->animation_accel_residual_valid ? 1 : 0;
    } else if (changed_selection && out.binding.runtime_id == 14) {
        runtime->speed_scalar = 5.0f;
        runtime->speed_scalar_valid = 1;
    }
    runtime->initialized = 1;
    runtime->previous_grounded = player->grounded ? 1 : 0;

    out.ticks_in_selection = player->sim_frame >= runtime->selection_entry_frame
        ? player->sim_frame - runtime->selection_entry_frame : 0;

    float frame_step = 0.5f;
    SA3DSSonicAnimationSpeedInputs speed_inputs = {
        local_forward_speed(player),
        runtime->speed_scalar
    };
    const SA3DSSonicAnimationSpeedInputs *inputs = &speed_inputs;
    if ((out.binding.property == 10 || out.binding.property == 11) &&
        !runtime->speed_scalar_valid)
        inputs = NULL;
    out.playback_rate_is_source_backed =
        sa3ds_sonic_animation_frame_step(&out.binding, inputs, &frame_step);
    /* Dynamic retail rates must be integrated per completed physics tick;
     * multiplying the current rate by total selection age rewrites history
     * whenever movement speed changes. */
    if (!changed_selection) {
        uint64_t elapsed = player->sim_frame - runtime->last_sim_frame;
        runtime->accumulated_sample_frame += (float)elapsed * frame_step;
    }
    out.sample_frame = runtime->accumulated_sample_frame;
    runtime->last_sim_frame = player->sim_frame;
    runtime->cached_selection = out;
    return out;
}
