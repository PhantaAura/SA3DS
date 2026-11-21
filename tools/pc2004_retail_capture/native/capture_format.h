#ifndef SA3DS_REFERENCE_CAPTURE_FORMAT_H
#define SA3DS_REFERENCE_CAPTURE_FORMAT_H
#include <stdint.h>
#pragma pack(push,1)
typedef struct {
 char magic[8];
 uint32_t version,header_bytes,record_bytes,record_count,act1_rows,errors,warmup_skipped,image_base,image_size,hook_mask;
 uint64_t qpc_start,qpc_frequency;
 char executable_sha256[64];
} CaptureHeader;
typedef struct {
 uint32_t frame,input_sequence,act1_ran;
 int32_t raw_x,raw_y;
 float filtered_magnitude;
 uint32_t filtered_angle,camera_yaw,held,pressed,released;
 float position[3],world_velocity[3],local_velocity[3];
 uint32_t task_angles[3],status,action,player_bits,player_low_status,terrain_flags,support_model;
 float acceleration[3],spindash_charge;
 uint32_t spindash_ticks;
 int32_t animation_requested,animation_source,animation_transition;
 float animation_frame;
 float collision_up[3],collision_normal[3],collision_normal2[3];
 uint32_t collision_count,collision_selected;
 uint32_t collision_valid,logical_held,logical_pressed,logical_released,camera_valid;
 uint64_t qpc;
} CaptureRecord;
#pragma pack(pop)
_Static_assert(sizeof(CaptureHeader)==128,"capture header layout");
_Static_assert(sizeof(CaptureRecord)==224,"capture record layout");
#endif
