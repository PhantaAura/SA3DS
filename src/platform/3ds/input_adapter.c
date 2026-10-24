#include "input_adapter.h"
#include "input_adapter_math.h"

SA1Input sa3ds_read_logical_input(u32 keys_down, u32 keys_held, u32 keys_up) {
    circlePosition circle;
    SA1Input input = {0};
    hidCircleRead(&circle);

    /*
     * Hardware adaptation only: map the 3DS Circle Pad range to -1..1.
     * Do NOT apply a second radial deadzone here. The portable Steam ground
     * kernel reproduces SADX's own component deadzone + cubic magnitude curve.
     * 156 is the practical libctru Circle Pad full-scale used by the lab; this
     * mapping is a 3DS hardware choice, not a claim about Dreamcast/PC ADCs.
     */
    const float device_max = 156.0f;
    SA3DSCirclePadSample sample =
        sa3ds_circle_pad_map(circle.dx, circle.dy, device_max);
    input.move_x = sample.normalized_x;
    input.move_z = sample.normalized_y;
    input.device_sample_valid = 1;
    input.device_raw_x = sample.raw_x;
    input.device_raw_y = sample.raw_y;
    input.device_normalized_magnitude = sample.normalized_magnitude;
    input.device_normalized_angle_bams = sample.normalized_angle_bams;
    input.device_full_scale = sample.full_scale;

    input.jump_pressed = (keys_down & KEY_A) != 0;
    input.jump_held = (keys_held & KEY_A) != 0;
    input.jump_released = (keys_up & KEY_A) != 0;

    const u32 action_mask = KEY_B | KEY_X;
    input.action_pressed = (keys_down & action_mask) != 0;
    input.action_held = (keys_held & action_mask) != 0;
    input.action_released = ((keys_up & action_mask) != 0) && !input.action_held;

    /* RESTORED profile shortcuts request existing actions; they never bypass
     * the SA1 action/state rules themselves. */
    input.spindash_shortcut_pressed = (keys_down & KEY_DDOWN) != 0;
    input.spindash_shortcut_held = (keys_held & KEY_DDOWN) != 0;
    input.spindash_shortcut_released = (keys_up & KEY_DDOWN) != 0;

    input.lightspeed_shortcut_pressed = (keys_down & KEY_DUP) != 0;
    input.lightspeed_shortcut_held = (keys_held & KEY_DUP) != 0;
    input.lightspeed_shortcut_released = (keys_up & KEY_DUP) != 0;

    return input;
}
