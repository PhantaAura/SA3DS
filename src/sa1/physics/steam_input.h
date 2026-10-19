#ifndef SA1_PHYSICS_STEAM_INPUT_H
#define SA1_PHYSICS_STEAM_INPUT_H

#include <stdint.h>

typedef struct {
    float post_deadzone_x;
    float post_deadzone_z;
    float magnitude;
    float angle_radians;
    uint16_t angle_bams;
    int active;
} SA1SADXFilteredAnalog;

/*
 * Reconstructs the Steam SADX controller shaping routine at 0x004C3FC0.
 * The caller supplies device-independent axes in -1..1. They are first mapped
 * onto the signed 8-bit range used by the PC game's peripheral data, then the
 * original component deadzone and cubic magnitude curve are applied.
 *
 * This preserves SADX's shaping while keeping the physical 3DS Circle Pad
 * calibration in the platform adapter.
 */
SA1SADXFilteredAnalog sa1_sadx_filter_logical_analog(float x, float z);

#endif
