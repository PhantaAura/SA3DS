#include "parity_trace.h"

#include <inttypes.h>

#include "../sa1/physics/movement.h"
#include "../sa1/physics/surface_frame.h"

int sa3ds_parity_trace_write_header(FILE *output) {
    if (!output) return -1;
    return fprintf(output,
        "trace_version,test_id,test_name,frame,move_x,move_z,"
        "jump_pressed,jump_held,jump_released,action_pressed,action_held,action_released,"
        "pos_x,pos_y,pos_z,vel_x,vel_y,vel_z,horizontal_speed,local_forward_speed,local_side_speed,heading,"
        "grounded,action_state,surface_id,surface_flags,normal_x,normal_y,normal_z,slope_radians,support_distance,"
        "runtime_animation_id,action_slot,animation_frame,selector_provisional\n") < 0 ? -1 : 0;
}

int sa3ds_parity_trace_write_frame(FILE *output,
    SA3DSParityZoneId test_id,
    const char *test_name,
    const SA1PlayerState *player,
    const SA1Input *input,
    const SA3DSSonicAnimationSelection *animation) {
    if (!output || !player || !input || !animation) return -1;
    float local_forward = sa1_vec3_dot(player->vel, player->surface_forward);
    float local_side = sa1_vec3_dot(player->vel, player->surface_right);
    int result = fprintf(output,
        "%d,%d,%s,%" PRIu64 ",%.9g,%.9g,"
        "%d,%d,%d,%d,%d,%d,"
        "%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,"
        "%d,%d,%" PRId32 ",%" PRIu32 ",%.9g,%.9g,%.9g,%.9g,%.9g,"
        "%u,%u,%.9g,%d\n",
        SA3DS_PARITY_TRACE_VERSION, (int)test_id, test_name ? test_name : "UNKNOWN", player->sim_frame,
        input->move_x, input->move_z,
        input->jump_pressed ? 1 : 0, input->jump_held ? 1 : 0, input->jump_released ? 1 : 0,
        input->action_pressed ? 1 : 0, input->action_held ? 1 : 0, input->action_released ? 1 : 0,
        player->pos.x, player->pos.y, player->pos.z,
        player->vel.x, player->vel.y, player->vel.z,
        sa1_horizontal_speed(player), local_forward, local_side, player->yaw,
        player->grounded ? 1 : 0, (int)player->action_state,
        player->surface_id, player->surface_flags,
        player->ground_normal.x, player->ground_normal.y, player->ground_normal.z,
        player->ground_slope_radians, player->support_distance,
        (unsigned)animation->binding.runtime_id, (unsigned)animation->binding.action_slot,
        animation->sample_frame, animation->selector_is_provisional ? 1 : 0);
    return result < 0 ? -1 : 0;
}
