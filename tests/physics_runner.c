#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "movement.h"
#include "physics_trace.h"
#include "steam_flat.h"
#include "stage.h"

static const char *SCENARIOS[] = {
    "accel_flat", "coast_from_speed", "brake_from_speed", "reverse_from_speed",
    "jump_tap", "jump_hold", "air_control", "gentle_uphill", "gentle_downhill",
    "steep_uphill", "bank_transition", "high_speed_turn", "low_speed_turn", "fall_reset",
    "spindash_action", "spindash_shortcut", "loop_roll",
    "loop_slow_wall_support", "loop_fast_wall_stick"
};

static int known_scenario(const char *name) {
    size_t count = sizeof(SCENARIOS) / sizeof(SCENARIOS[0]);
    for (size_t i = 0; i < count; ++i) if (!strcmp(name, SCENARIOS[i])) return 1;
    return 0;
}

static void scenario_setup(const char *name, SA1PlayerState *player) {
    if (!strcmp(name, "gentle_uphill")) { player->pos.z = 52.0f; player->pos.y = 1.5882353f; }
    else if (!strcmp(name, "gentle_downhill")) { player->pos.z = 78.0f; player->pos.y = 5.4117646f; player->yaw = 3.14159265f; }
    else if (!strcmp(name, "steep_uphill")) { player->pos.z = 84.0f; player->pos.y = 6.8461537f; }
    else if (!strcmp(name, "bank_transition")) { player->pos.z = 140.0f; player->pos.y = 17.0f; }
    else if (!strcmp(name, "fall_reset")) { player->pos.y = -36.0f; player->grounded = false; }
    else if (!strcmp(name, "loop_roll")) {
        player->pos = (SA1Vec3){0.0f, 5.0f, 225.0f};
        player->vel = (SA1Vec3){0.0f, 0.0f, 4.5f};
        player->yaw = 0.0f;
        player->grounded = true;
        player->ground_normal = (SA1Vec3){0.0f, 1.0f, 0.0f};
        player->surface_forward = (SA1Vec3){0.0f, 0.0f, 1.0f};
        player->surface_right = (SA1Vec3){1.0f, 0.0f, 0.0f};
        player->action_state = SA1_ACTION_ROLL;
        player->ball_form = true;
        player->surface_id = 100;
    }
    else if (!strcmp(name, "loop_slow_wall_support") ||
             !strcmp(name, "loop_fast_wall_stick")) {
        player->pos = (SA1Vec3){0.0f, 18.0f, 238.0f};
        player->vel = (SA1Vec3){0.0f,
            !strcmp(name, "loop_slow_wall_support") ? 0.90f : 2.00f, 0.0f};
        player->yaw = 0.0f;
        player->grounded = true;
        player->ground_normal = (SA1Vec3){0.0f, 0.0f, -1.0f};
        player->surface_forward = (SA1Vec3){0.0f, 1.0f, 0.0f};
        player->surface_right = (SA1Vec3){1.0f, 0.0f, 0.0f};
        player->action_state = SA1_ACTION_ROLL;
        player->ball_form = true;
        player->surface_id = 100;
    }
}

static SA1Input scenario_input(const char *name, unsigned frame) {
    SA1Input input = {0};
    if (!strcmp(name, "accel_flat") || !strcmp(name, "gentle_uphill") ||
        !strcmp(name, "steep_uphill") || !strcmp(name, "bank_transition")) input.move_z = 1.0f;
    else if (!strcmp(name, "gentle_downhill")) input.move_z = -1.0f;
    else if (!strcmp(name, "coast_from_speed")) input.move_z = frame < 60 ? 1.0f : 0.0f;
    else if (!strcmp(name, "brake_from_speed"))
        input.move_z = frame < 60 ? 1.0f : (frame < 90 ? -1.0f : 0.0f);
    else if (!strcmp(name, "reverse_from_speed")) input.move_z = frame < 60 ? 1.0f : -1.0f;
    else if (!strcmp(name, "jump_tap") || !strcmp(name, "jump_hold")) {
        input.jump_pressed = frame == 0;
        input.jump_held = !strcmp(name, "jump_hold") ? frame < 12 : frame == 0;
    } else if (!strcmp(name, "air_control")) {
        input.jump_pressed = frame == 0; input.jump_held = frame < 8; input.move_x = frame > 2 ? 1.0f : 0.0f;
    } else if (!strcmp(name, "high_speed_turn")) {
        input.move_z = frame < 80 ? 1.0f : 0.0f; input.move_x = frame >= 80 ? 1.0f : 0.0f;
    } else if (!strcmp(name, "low_speed_turn")) {
        input.move_z = frame < 8 ? 0.35f : 0.0f; input.move_x = frame >= 8 ? 0.35f : 0.0f;
    } else if (!strcmp(name, "spindash_action")) {
        input.action_pressed = frame == 0;
        input.action_held = frame < 20;
        input.action_released = frame == 20;
    } else if (!strcmp(name, "spindash_shortcut")) {
        input.spindash_shortcut_pressed = frame == 0;
        input.spindash_shortcut_held = frame < 20;
        input.spindash_shortcut_released = frame == 20;
    }
    return input;
}

int main(int argc, char **argv) {
    if (argc != 4 || !known_scenario(argv[1])) {
        fprintf(stderr, "usage: %s SCENARIO FRAMES OUTPUT.csv\n", argv[0]);
        return 2;
    }
    char *end = 0;
    unsigned long frames = strtoul(argv[2], &end, 10);
    if (!end || *end || frames == 0 || frames > 1000000ul) return 2;
    FILE *output = fopen(argv[3], "wb");
    if (!output) { perror(argv[3]); return 1; }

    SA1PlayerState player;
    sa1_player_reset(&player, sa3ds_movement_lab_world());
    scenario_setup(argv[1], &player);
    if (sa1_trace_write_header(output)) return 1;
    for (unsigned long frame = 0; frame < frames; ++frame) {
        SA1Input input = scenario_input(argv[1], (unsigned)frame);
        sa1_physics_step_steam_sonic_flat(&player, &input, 0.0f,
                         sa3ds_movement_lab_world(),
                         &SA1_PHYSICS_PLACEHOLDER_CONSTANTS, NULL);
        if (sa1_trace_write_frame(output, &player, &input)) return 1;
    }
    if (fclose(output)) return 1;
    printf("trace: %s (%lu frames) -> %s\n", argv[1], frames, argv[3]);
    return 0;
}
