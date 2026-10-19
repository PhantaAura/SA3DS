#ifndef SA3DS_SONIC_ACTION_PREFLIGHT_H
#define SA3DS_SONIC_ACTION_PREFLIGHT_H

#include <stddef.h>
#include <stdint.h>
#include "sonic_animation_runtime.h"
#include "../sa3c/sa3c_pose.h"

typedef enum {
    SA3DS_ACTION_PREFLIGHT_OK = 0,
    SA3DS_ACTION_PREFLIGHT_ARGUMENT,
    SA3DS_ACTION_PREFLIGHT_SLOT,
    SA3DS_ACTION_PREFLIGHT_EMPTY,
    SA3DS_ACTION_PREFLIGHT_METADATA,
    SA3DS_ACTION_PREFLIGHT_POSE,
    SA3DS_ACTION_PREFLIGHT_NONFINITE,
    SA3DS_ACTION_PREFLIGHT_TRACK_RANGE,
    SA3DS_ACTION_PREFLIGHT_TRACK_EMPTY,
    SA3DS_ACTION_PREFLIGHT_KEY_ORDER
} SA3DSActionPreflightResult;

typedef struct {
    uint32_t checked_bindings;
    uint32_t checked_samples;
    uint16_t failing_runtime_id;
    uint16_t failing_action_slot;
    float failing_frame;
    SA3CPoseResult pose_error;
    uint32_t failing_channel_index; /* UINT32_MAX when no channel is implicated */
    uint32_t failing_key_index;     /* UINT32_MAX when no key is implicated */
    uint8_t failing_track;         /* 1=position, 2=rotation, 4=scale; 0=none */
} SA3DSActionPreflightReport;

/* Validate the exact source-backed runtime->ACTION bindings against an owner-
 * generated SA3C package. No Sega bytes are embedded. This checks whether
 * frames can be sampled safely; it does not validate retail clip timing. */
SA3DSActionPreflightResult sa3ds_action_preflight(
    const SA3CCharacter *character,
    const SA3DSSonicAnimationBinding *bindings, size_t binding_count,
    SA3DSActionPreflightReport *report);

const char *sa3ds_action_preflight_result_name(SA3DSActionPreflightResult result);

#endif
