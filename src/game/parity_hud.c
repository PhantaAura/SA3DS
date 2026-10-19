#include "parity_hud.h"

#include <math.h>
#include <string.h>

#include "../sa1/physics/movement.h"

#define SA3DS_PI 3.14159265358979323846f

static float wrap_angle(float a) {
    while (a > SA3DS_PI) a -= 2.0f * SA3DS_PI;
    while (a < -SA3DS_PI) a += 2.0f * SA3DS_PI;
    return a;
}

void sa3ds_parity_camera_init(SA3DSParityCameraState *camera, float yaw) {
    if (!camera) return;
    camera->mode = SA3DS_PARITY_CAMERA_FOLLOW;
    camera->yaw = wrap_angle(yaw);
}

void sa3ds_parity_camera_toggle(SA3DSParityCameraState *camera) {
    if (!camera) return;
    camera->mode = camera->mode == SA3DS_PARITY_CAMERA_FOLLOW
        ? SA3DS_PARITY_CAMERA_REFERENCE : SA3DS_PARITY_CAMERA_FOLLOW;
}

void sa3ds_parity_camera_recenter(SA3DSParityCameraState *camera, float player_yaw) {
    if (!camera) return;
    camera->yaw = wrap_angle(player_yaw);
}

void sa3ds_parity_camera_manual_yaw(SA3DSParityCameraState *camera, float delta) {
    if (!camera) return;
    camera->yaw = wrap_angle(camera->yaw + delta);
}

void sa3ds_parity_camera_tick(SA3DSParityCameraState *camera, float player_yaw,
                              int manual_yaw_active) {
    if (!camera || manual_yaw_active || camera->mode != SA3DS_PARITY_CAMERA_FOLLOW) return;
    float difference = wrap_angle(player_yaw - camera->yaw);
    camera->yaw = wrap_angle(camera->yaw + difference * 0.025f);
}

const char *sa3ds_parity_camera_mode_name(SA3DSParityCameraMode mode) {
    return mode == SA3DS_PARITY_CAMERA_REFERENCE ? "REFERENCE" : "FOLLOW";
}

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
    SA3DSParityCameraMode camera_mode) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->active_zone = active ? active->name : "OUTSIDE";
    out->selected_zone = selected ? selected->name : "?";
    out->animation_frame_count = animation_frame_count;
    out->trace_recording = trace_recording ? 1 : 0;
    out->trace_frames = trace_frames;
    out->draw_calls = draw_calls;
    out->triangles = triangles;
    out->free_linear_bytes = free_linear_bytes;
    out->camera_mode = camera_mode;
    if (!player) return;
    out->sim_frame = player->sim_frame;
    out->pos_x = player->pos.x; out->pos_y = player->pos.y; out->pos_z = player->pos.z;
    out->vel_x = player->vel.x; out->vel_y = player->vel.y; out->vel_z = player->vel.z;
    out->horizontal_speed = sa1_horizontal_speed(player);
    out->heading = player->yaw;
    out->grounded = player->grounded ? 1 : 0;
    out->action_state = (int)player->action_state;
    out->surface_id = player->surface_id;
    out->normal_x = player->ground_normal.x;
    out->normal_y = player->ground_normal.y;
    out->normal_z = player->ground_normal.z;
    out->slope_radians = player->ground_slope_radians;
    out->support_distance = player->support_distance;
    if (animation) {
        out->runtime_animation_id = animation->binding.runtime_id;
        out->action_slot = animation->binding.action_slot;
        out->animation_frame = animation->sample_frame;
        out->selector_is_provisional = animation->selector_is_provisional;
    }
}
