#include "physics_trace.h"
#include "movement.h"
#include "steam_input.h"
#include "surface_frame.h"
#include <inttypes.h>

int sa1_trace_write_header(FILE *output) {
    return fprintf(output,
        "trace_version,frame,move_x,move_z,filtered_input_magnitude,filtered_input_angle_bams,"
        "jump_pressed,jump_held,jump_released,"
        "action_pressed,action_held,action_released,"
        "spindash_shortcut_pressed,spindash_shortcut_held,spindash_shortcut_released,"
        "lightspeed_shortcut_pressed,lightspeed_shortcut_held,lightspeed_shortcut_released,"
        "pos_x,pos_y,pos_z,vel_x,vel_y,vel_z,horizontal_speed,local_forward_speed,local_side_speed,grounded,"
        "normal_x,normal_y,normal_z,slope_radians,action_state,jump_ticks,surface_id,"
        "ball_form,spindash_charge,spindash_ticks,"
        "surface_forward_x,surface_forward_y,surface_forward_z,"
        "surface_right_x,surface_right_y,surface_right_z,"
        "support_distance,surface_flags,surface_tangent_speed\n") < 0 ? -1 : 0;
}

int sa1_trace_write_frame(FILE *output, const SA1PlayerState *player,
                          const SA1Input *input) {
    SA1SADXFilteredAnalog analog =
        sa1_sadx_filter_logical_analog(input->move_x, input->move_z);
    float local_forward = sa1_vec3_dot(player->vel, player->surface_forward);
    float local_side = sa1_vec3_dot(player->vel, player->surface_right);

    int result = fprintf(output,
        "%d,%" PRIu64 ",%.9g,%.9g,%.9g,%u,"
        "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
        "%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,"
        "%.9g,%.9g,%.9g,%d,%.9g,%.9g,%.9g,%.9g,%d,%" PRIu32 ",%" PRId32 ","
        "%d,%.9g,%" PRIu32 ","
        "%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%" PRIu32 ",%.9g\n",
        SA1_TRACE_FORMAT_VERSION, player->sim_frame,
        input->move_x, input->move_z, analog.magnitude, (unsigned)analog.angle_bams,
        input->jump_pressed ? 1 : 0, input->jump_held ? 1 : 0, input->jump_released ? 1 : 0,
        input->action_pressed ? 1 : 0, input->action_held ? 1 : 0, input->action_released ? 1 : 0,
        input->spindash_shortcut_pressed ? 1 : 0,
        input->spindash_shortcut_held ? 1 : 0,
        input->spindash_shortcut_released ? 1 : 0,
        input->lightspeed_shortcut_pressed ? 1 : 0,
        input->lightspeed_shortcut_held ? 1 : 0,
        input->lightspeed_shortcut_released ? 1 : 0,
        player->pos.x, player->pos.y, player->pos.z,
        player->vel.x, player->vel.y, player->vel.z,
        sa1_horizontal_speed(player), local_forward, local_side,
        player->grounded ? 1 : 0,
        player->ground_normal.x, player->ground_normal.y, player->ground_normal.z,
        player->ground_slope_radians, (int)player->action_state,
        player->jump_ticks, player->surface_id,
        player->ball_form ? 1 : 0, player->spindash_charge, player->spindash_ticks,
        player->surface_forward.x, player->surface_forward.y, player->surface_forward.z,
        player->surface_right.x, player->surface_right.y, player->surface_right.z,
        player->support_distance, player->surface_flags,
        sa1_surface_tangent_speed(player));
    return result < 0 ? -1 : 0;
}
