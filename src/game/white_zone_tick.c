#include "white_zone_tick.h"
#include <string.h>

void sa3ds_wz_tick_reset(SA3DSWhiteZoneTick *clock) {
    if (clock) memset(clock, 0, sizeof(*clock));
}

unsigned sa3ds_wz_tick_begin(SA3DSWhiteZoneTick *clock,
                             uint32_t elapsed_ms, const SA1Input *input) {
    if (!clock || !input) return 0;
    /* Accumulate button edges even when rendering outruns simulation. */
    SA1Input *p = &clock->pending_input;
    p->jump_pressed |= input->jump_pressed;
    p->jump_released |= input->jump_released;
    p->action_pressed |= input->action_pressed;
    p->action_released |= input->action_released;
    p->spindash_shortcut_pressed |= input->spindash_shortcut_pressed;
    p->spindash_shortcut_released |= input->spindash_shortcut_released;
    p->lightspeed_shortcut_pressed |= input->lightspeed_shortcut_pressed;
    p->lightspeed_shortcut_released |= input->lightspeed_shortcut_released;
    p->move_x = input->move_x;
    p->move_z = input->move_z;
    p->device_sample_valid = input->device_sample_valid;
    p->device_raw_x = input->device_raw_x;
    p->device_raw_y = input->device_raw_y;
    p->device_normalized_magnitude = input->device_normalized_magnitude;
    p->device_normalized_angle_bams = input->device_normalized_angle_bams;
    p->device_full_scale = input->device_full_scale;
    p->jump_held = input->jump_held;
    p->action_held = input->action_held;
    p->spindash_shortcut_held = input->spindash_shortcut_held;
    p->lightspeed_shortcut_held = input->lightspeed_shortcut_held;
    clock->pending_edges = 1;

    /* Ignore wall-clock time beyond a 250 ms stall (sleep/lifecycle). */
    if (elapsed_ms > SA3DS_WZ_MAX_ELAPSED_MS) elapsed_ms = SA3DS_WZ_MAX_ELAPSED_MS;
    clock->phase += elapsed_ms * SA3DS_WZ_TICKS_PER_SECOND;
    unsigned steps = clock->phase / SA3DS_WZ_PHASE_PER_TICK;
    clock->phase %= SA3DS_WZ_PHASE_PER_TICK;
    if (steps > SA3DS_WZ_MAX_STEPS_PER_FRAME) {
        clock->dropped_ticks += steps - SA3DS_WZ_MAX_STEPS_PER_FRAME;
        steps = SA3DS_WZ_MAX_STEPS_PER_FRAME;
    }
    clock->total_ticks += steps;
    return steps;
}

SA1Input sa3ds_wz_tick_input(SA3DSWhiteZoneTick *clock, unsigned step_index) {
    SA1Input input = {0};
    if (!clock) return input;
    input = clock->pending_input;
    if (step_index == 0) {
        /* Consume latched edges after they have been delivered to physics. */
        clock->pending_input.jump_pressed = false;
        clock->pending_input.jump_released = false;
        clock->pending_input.action_pressed = false;
        clock->pending_input.action_released = false;
        clock->pending_input.spindash_shortcut_pressed = false;
        clock->pending_input.spindash_shortcut_released = false;
        clock->pending_input.lightspeed_shortcut_pressed = false;
        clock->pending_input.lightspeed_shortcut_released = false;
        clock->pending_edges = 0;
    } else {
        input.jump_pressed = input.jump_released = false;
        input.action_pressed = input.action_released = false;
        input.spindash_shortcut_pressed = input.spindash_shortcut_released = false;
        input.lightspeed_shortcut_pressed = input.lightspeed_shortcut_released = false;
    }
    return input;
}
