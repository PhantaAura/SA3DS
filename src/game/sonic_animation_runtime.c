#include "sonic_animation_runtime.h"

#include <math.h>
#include <string.h>

static const SA3DSSonicAnimationBinding kBindings[] = {
    /* Exact scalar fields recovered from PC2004 InitSonicAnimData (0x7CEC90).
     * Property meanings are verified against PlayAnimation (0x44A800). */
    { 0,  1, 73,  3,  0, 0.0625f, 0.5f, 72, 32, "Stand"},
    { 9,  3, 73, 10,  9, 0.25f,   3.4f, 72, 32, "Walk1"},
    {10,  4, 73, 10, 10, 0.25f,   1.4f, 72, 32, "Walk2"},
    {11,  5, 73, 10, 11, 0.25f,   0.8f, 72, 32, "Walk3"},
    {12,  6, 73, 10, 12, 0.25f,   0.5f, 72, 32, "Run1"},
    {13, 18, 73,  9, 13, 0.5f,    0.5f, 72, 33, "Run2"},
    {14, 14, 74, 10, 14, 0.25f,   0.2f,146,  7, "Jump/Spin"},
    {15, 14, 74,  9, 15, 0.25f,   0.7f,146,  7, "Roll"},
    {16,  8, 73,  3, 16, 0.25f,   1.0f, 72, 32, "Spring"},
    {17,  9, 73,  4, 18, 0.125f,  1.0f, 72, 33, "Spring->Fall"},
    {18, 10, 73,  3, 18, 0.25f,   1.0f, 72, 32, "Fall"},
    {19, 11, 73,  4,  0, 1.0f,    1.0f, 72, 16, "Land"},
    {20,  7, 73,  4,  0, 0.25f,   1.0f, 72, 33, "Brake"},
    {29, 22, 73,  4,  1, 0.0625f, 1.0f, 72, 16, "Uncurl"},
    {30, 20, 73,  4,  0, 1.0f,    0.9f, 72, 16, "Turn Around"},
    {31, 19, 73,  3, 31, 0.0625f, 1.0f, 72, 16, "Skidding"},
    {32, 21,  1,  3, 32, 0.5f,    1.0f,147,  7, "Spin Ball"},
};

static float local_forward_speed(const SA1PlayerState *player);

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
    if (player->grounded && !previous_grounded)
        return 19;

    switch (o->intent) {
    case SA3DS_SONIC_ANIM_GROUNDED_IDLE:
        return 0;
    case SA3DS_SONIC_ANIM_GROUNDED_BELOW_JOG:
    case SA3DS_SONIC_ANIM_GROUNDED_JOG_TO_RUN:
    case SA3DS_SONIC_ANIM_GROUNDED_RUN_PLUS: {
        /* DoSonicGroundAnimation's selector at 0x00495CD0 uses the signed
         * local speed plus five times the acceleration residual. */
        float speed = local_forward_speed(player) +
            player->animation_accel_residual * 5.0f;
        if (speed <= SA1_STEAM_SONIC_PARAMETER_EVIDENCE.jog_speed) return 9;
        if (speed <= SA1_STEAM_SONIC_PARAMETER_EVIDENCE.run_speed) return 10;
        if (speed <= SA1_STEAM_SONIC_PARAMETER_EVIDENCE.rush_speed) return 11;
        if (speed <= SA1_STEAM_SONIC_PARAMETER_EVIDENCE.dash_speed) return 12;
        return 13;
    }
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

static void select_binding(SA3DSSonicAnimationRuntime *runtime,
                           uint16_t runtime_id, uint64_t sim_frame) {
    if (!sa3ds_sonic_animation_binding(runtime_id, &runtime->active_binding))
        (void)sa3ds_sonic_animation_binding(0, &runtime->active_binding);
    runtime->selection_entry_frame = sim_frame;
    runtime->accumulated_sample_frame = 0.0f;
}

static float current_frame_step(SA3DSSonicAnimationRuntime *runtime,
                                const SA1PlayerState *player,
                                int *source_backed) {
    SA3DSSonicAnimationSpeedInputs inputs = {
        local_forward_speed(player), runtime->speed_scalar
    };
    const SA3DSSonicAnimationSpeedInputs *input_ptr = &inputs;
    if ((runtime->active_binding.property == 10 ||
         runtime->active_binding.property == 11) &&
        !runtime->speed_scalar_valid)
        input_ptr = NULL;
    float step = 0.5f;
    *source_backed = sa3ds_sonic_animation_frame_step(
        &runtime->active_binding, input_ptr, &step);
    return step;
}

SA3DSSonicAnimationSelection sa3ds_sonic_animation_select(
    SA3DSSonicAnimationRuntime *runtime,
    const SA1PlayerState *player,
    const SA3DSSonicAnimationObservation *observation) {
    SA3DSSonicAnimationSelection out;
    memset(&out, 0, sizeof(out));
    out.selector_is_provisional = 1;
    if (!runtime || !player || !observation) return out;

    /* One completed physics tick advances transition/property state once.
     * Renderer/HUD queries in the same tick must retain the same pose. */
    if (runtime->initialized) {
        if (player->sim_frame == runtime->last_sim_frame)
            return runtime->cached_selection;
        if (player->sim_frame < runtime->last_sim_frame)
            *runtime = (SA3DSSonicAnimationRuntime){0};
    }

    int previous_grounded = runtime->initialized ? runtime->previous_grounded
                                                  : player->grounded;
    uint16_t requested_id = choose_runtime_id(player, observation,
                                               previous_grounded);

    /* The second retail pointer targets stateful CharObj2.SpindashSpeed.
     * Ground locomotion refreshes it every selector call. The jump transition
     * that selects action 14 instead assigns 5.0 and leaves it persistent. */
    if (requested_id >= 9 && requested_id <= 13) {
        runtime->speed_scalar = local_forward_speed(player) +
            player->animation_accel_residual * 20.0f;
        runtime->speed_scalar_valid =
            player->animation_accel_residual_valid ? 1 : 0;
    } else if ((!runtime->initialized ||
                runtime->active_binding.runtime_id != requested_id) &&
               requested_id == 14) {
        runtime->speed_scalar = 5.0f;
        runtime->speed_scalar_valid = 1;
    }

    if (!runtime->initialized) {
        select_binding(runtime, requested_id, player->sim_frame);
        runtime->initialized = 1;
    } else {
        uint64_t elapsed = player->sim_frame - runtime->last_sim_frame;
        for (uint64_t tick = 0; tick < elapsed; ++tick) {
            if (runtime->transition_active) {
                runtime->transition_progress +=
                    runtime->active_binding.transition_speed;
                if (runtime->transition_progress >= 1.0f) {
                    runtime->transition_active = 0;
                    runtime->transition_progress = 0.0f;
                    runtime->accumulated_sample_frame = 0.0f;
                }
                continue;
            }

            /* Property 4 owns Index until its source action reaches
             * frames-1, then installs NextAnim immediately with no blend. */
            if (runtime->active_binding.property == 4) {
                int backed = 0;
                runtime->accumulated_sample_frame +=
                    current_frame_step(runtime, player, &backed);
                if (runtime->active_binding.source_frame_count > 0 &&
                    runtime->accumulated_sample_frame >=
                        (float)(runtime->active_binding.source_frame_count - 1)) {
                    select_binding(runtime,
                        runtime->active_binding.next_runtime_id,
                        player->sim_frame);
                }
                continue;
            }

            if (requested_id != runtime->active_binding.runtime_id) {
                SA3DSSonicAnimationBinding destination;
                if (!sa3ds_sonic_animation_binding(requested_id, &destination))
                    (void)sa3ds_sonic_animation_binding(0, &destination);
                if (destination.transition_speed < 1.0f &&
                    destination.source_root_node ==
                        runtime->active_binding.source_root_node) {
                    runtime->transition_from_action_slot =
                        runtime->active_binding.action_slot;
                    runtime->transition_from_frame =
                        runtime->accumulated_sample_frame;
                    runtime->active_binding = destination;
                    runtime->selection_entry_frame = player->sim_frame;
                    runtime->accumulated_sample_frame = 0.0f;
                    runtime->transition_progress = destination.transition_speed;
                    runtime->transition_active = 1;
                } else {
                    select_binding(runtime, requested_id, player->sim_frame);
                }
                continue;
            }

            int backed = 0;
            runtime->accumulated_sample_frame +=
                current_frame_step(runtime, player, &backed);
        }
    }

    runtime->previous_grounded = player->grounded ? 1 : 0;
    out.binding = runtime->active_binding;
    out.ticks_in_selection = player->sim_frame >= runtime->selection_entry_frame
        ? player->sim_frame - runtime->selection_entry_frame : 0;
    int backed = 0;
    (void)current_frame_step(runtime, player, &backed);
    out.playback_rate_is_source_backed = backed;
    out.sample_frame = runtime->accumulated_sample_frame;
    out.transition_active = runtime->transition_active;
    out.transition_from_action_slot = runtime->transition_from_action_slot;
    out.transition_from_frame = runtime->transition_from_frame;
    out.transition_weight = runtime->transition_progress;
    runtime->last_sim_frame = player->sim_frame;
    runtime->cached_selection = out;
    return out;
}
