#ifndef SA3DS_PLATFORM_INPUT_ADAPTER_MATH_H
#define SA3DS_PLATFORM_INPUT_ADAPTER_MATH_H

#include <stdint.h>

typedef struct {
    int16_t raw_x;
    int16_t raw_y;
    float normalized_x;
    float normalized_y;
    float normalized_magnitude;
    uint16_t normalized_angle_bams;
    float full_scale;
} SA3DSCirclePadSample;

/* Hardware adaptation only. No PC2004 deadzone or response curve is applied. */
SA3DSCirclePadSample sa3ds_circle_pad_map(int16_t raw_x, int16_t raw_y,
                                          float full_scale);

#endif
