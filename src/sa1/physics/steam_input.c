#include "steam_input.h"
#include <math.h>

#define SA1_PI 3.14159265358979323846f
#define SA1_TAU (2.0f * SA1_PI)
#define SADX_AXIS_POS_MAX 32512.0f /* 127 << 8 */
#define SADX_AXIS_NEG_MAX 32768.0f /* 128 << 8 */
#define SADX_COMPONENT_DEADZONE 3072.0f
#define SADX_POST_DEADZONE_SCALE 29441.0f

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float logical_to_sadx_axis(float v) {
    v = clampf(v, -1.0f, 1.0f);
    return v >= 0.0f ? v * SADX_AXIS_POS_MAX : v * SADX_AXIS_NEG_MAX;
}

static float strip_component_deadzone(float v) {
    if (v >= SADX_COMPONENT_DEADZONE) return v - SADX_COMPONENT_DEADZONE;
    if (v <= -SADX_COMPONENT_DEADZONE) return v + SADX_COMPONENT_DEADZONE;
    return 0.0f;
}

static uint16_t radians_to_bams(float a) {
    float turns = a / SA1_TAU;
    long value = lroundf(turns * 65536.0f);
    return (uint16_t)value;
}

SA1SADXFilteredAnalog sa1_sadx_filter_logical_analog(float x, float z) {
    SA1SADXFilteredAnalog out = {0};
    float sx = strip_component_deadzone(logical_to_sadx_axis(x));
    float sz = strip_component_deadzone(logical_to_sadx_axis(z));
    out.post_deadzone_x = sx;
    out.post_deadzone_z = sz;

    if (sx == 0.0f && sz == 0.0f) return out;

    out.active = 1;
    out.angle_radians = atan2f(sx, sz);
    out.angle_bams = radians_to_bams(out.angle_radians);

    /* Steam computes sqrt(r^2) * r^2 / scale^3, i.e. a cubic radial curve. */
    float r2 = sx * sx + sz * sz;
    float denom = SADX_POST_DEADZONE_SCALE * SADX_POST_DEADZONE_SCALE *
                  SADX_POST_DEADZONE_SCALE;
    out.magnitude = sqrtf(r2) * r2 / denom;
    if (out.magnitude > 1.0f) out.magnitude = 1.0f;
    return out;
}
