#ifndef SA1_PHYSICS_INPUT_H
#define SA1_PHYSICS_INPUT_H

#include <stdbool.h>

/*
 * Platform-independent, per-tick logical input.
 *
 * Physical buttons are translated by a platform adapter. Core physics never
 * reads 3DS input directly.
 *
 * ACTION is the normal contextual SA1 action family.
 *
 * The RESTORED shortcut requests below are deliberately separate from ACTION.
 * They are requests into the same eventual SA1 state machine; they must never
 * bypass charge requirements, collision, eligibility, or physics. Until those
 * original behaviors are verified, the portable movement core records but
 * otherwise ignores the requests.
 */
typedef struct {
    float move_x; /* camera-relative, nominal range -1..1 */
    float move_z; /* camera-relative, nominal range -1..1 */

    /* Optional platform-sample provenance. Physics consumes only move_x/z;
     * these fields let native traces distinguish hardware calibration from
     * the verified PC2004 response curve. Zero/invalid for host fixtures. */
    int device_sample_valid;
    int device_raw_x;
    int device_raw_y;
    float device_normalized_magnitude;
    unsigned device_normalized_angle_bams;
    float device_full_scale;

    bool jump_pressed;
    bool jump_held;
    bool jump_released;

    bool action_pressed;
    bool action_held;
    bool action_released;

    /* Restored-control requests. */
    bool spindash_shortcut_pressed;
    bool spindash_shortcut_held;
    bool spindash_shortcut_released;

    bool lightspeed_shortcut_pressed;
    bool lightspeed_shortcut_held;
    bool lightspeed_shortcut_released;
} SA1Input;

#endif
