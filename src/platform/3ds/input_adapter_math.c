#include "input_adapter_math.h"

#include <math.h>

#define SA3DS_TAU 6.28318530717958647692f

static float clampf(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

SA3DSCirclePadSample sa3ds_circle_pad_map(int16_t raw_x, int16_t raw_y,
                                          float full_scale) {
    SA3DSCirclePadSample out = {0};
    out.raw_x = raw_x;
    out.raw_y = raw_y;
    out.full_scale = full_scale;
    if (!(full_scale > 0.0f) || !isfinite(full_scale)) return out;

    out.normalized_x = clampf((float)raw_x / full_scale, -1.0f, 1.0f);
    out.normalized_y = clampf((float)raw_y / full_scale, -1.0f, 1.0f);
    out.normalized_magnitude = sqrtf(out.normalized_x * out.normalized_x +
                                      out.normalized_y * out.normalized_y);
    if (out.normalized_x != 0.0f || out.normalized_y != 0.0f) {
        float turns = atan2f(out.normalized_x, out.normalized_y) / SA3DS_TAU;
        out.normalized_angle_bams = (uint16_t)lroundf(turns * 65536.0f);
    }
    return out;
}
