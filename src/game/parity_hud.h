#ifndef SA3DS_GAME_PARITY_HUD_H
#define SA3DS_GAME_PARITY_HUD_H

#include <stddef.h>
#include <stdint.h>

#include "parity_lab.h"
#include "sonic_animation_runtime.h"
#include "../sa1/physics/player_state.h"

typedef enum {
    SA3DS_PARITY_CAMERA_FOLLOW = 0,
    SA3DS_PARITY_CAMERA_REFERENCE = 1
} SA3DSParityCameraMode;

typedef struct {
    SA3DSParityCameraMode mode;
    float yaw;
} SA3DSParityCameraState;

typedef struct {
    const char *active_zone;
    const char *selected_zone;
    uint64_t sim_frame;
    float pos_x, pos_y, pos_z;
    float vel_x, vel_y, vel_z;
    float horizontal_speed;
    float heading;
    int grounded;
    int action_state;
    int32_t surface_id;
    float normal_x, normal_y, normal_z;
    float slope_radians;
    float support_distance;
    uint16_t runtime_animation_id;
    uint16_t action_slot;
    float animation_frame;
    uint16_t animation_frame_count;
    int selector_is_provisional;
    int trace_recording;
    unsigned trace_frames;
    unsigned draw_calls;
    unsigned triangles;
    size_t free_linear_bytes;
    SA3DSParityCameraMode camera_mode;
} SA3DSParityHudSnapshot;

void sa3ds_parity_camera_init(SA3DSParityCameraState *camera, float yaw);
void sa3ds_parity_camera_toggle(SA3DSParityCameraState *camera);
void sa3ds_parity_camera_recenter(SA3DSParityCameraState *camera, float player_yaw);
void sa3ds_parity_camera_manual_yaw(SA3DSParityCameraState *camera, float delta);
void sa3ds_parity_camera_tick(SA3DSParityCameraState *camera, float player_yaw,
                              int manual_yaw_active);
const char *sa3ds_parity_camera_mode_name(SA3DSParityCameraMode mode);

void sa3ds_parity_hud_snapshot(SA3DSParityHudSnapshot *out,
    const SA1PlayerState *player,
    const SA3DSParityZone *active,
    const SA3DSParityZone *selected,
    const SA3DSSonicAnimationSelection *animation,
    uint16_t animation_frame_count,
    int trace_recording,
    unsigned trace_frames,
    unsigned draw_calls,
    unsigned triangles,
    size_t free_linear_bytes,
    SA3DSParityCameraMode camera_mode);

#endif
