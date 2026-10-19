#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "movement.h"
#include "stage.h"

static void forward(SA1PlayerState *player, int ticks) {
    SA1Input input = {0}; input.move_z = 1.0f;
    for (int i = 0; i < ticks; ++i)
        sa1_physics_step(player, &input, 0.0f, sa3ds_movement_lab_world(),
                         &SA1_PHYSICS_PLACEHOLDER_CONSTANTS);
}

int main(void) {
    SA1PlayerState player;
    sa1_player_reset(&player, sa3ds_movement_lab_world());
    forward(&player, 120);
    /* PLACEHOLDER EXPECTATIONS: regress project behavior, not SA1 reference data. */
    assert(sa1_horizontal_speed(&player) > 0.4f);
    assert(sa1_horizontal_speed(&player) <= 1.701f);

    sa1_player_reset(&player, sa3ds_movement_lab_world());
    SA1Input jump = {0}; jump.jump_pressed = true; jump.jump_held = true;
    sa1_physics_step(&player, &jump, 0.0f, sa3ds_movement_lab_world(),
                     &SA1_PHYSICS_PLACEHOLDER_CONSTANTS);
    for (int i = 0; i < 120; ++i) {
        SA1Input hold = {0}; hold.jump_held = i < 12;
        sa1_physics_step(&player, &hold, 0.0f, sa3ds_movement_lab_world(),
                         &SA1_PHYSICS_PLACEHOLDER_CONSTANTS);
    }
    assert(player.grounded);

    SA1Surface up = sa3ds_movement_lab_surface(0, 0.0f, 60.0f);
    SA1Surface bank = sa3ds_movement_lab_surface(0, 10.0f, 160.0f);
    assert(up.valid && up.grad_z > 0.0f && up.surface_id == 2);
    assert(bank.valid && fabsf(bank.grad_x) > 0.0f && bank.surface_id == 5);

    player.pos.y = -100.0f;
    SA1Input none = {0};
    sa1_physics_step(&player, &none, 0.0f, sa3ds_movement_lab_world(),
                     &SA1_PHYSICS_PLACEHOLDER_CONSTANTS);
    assert(player.pos.z < 0.0f); /* reset near start */

    SA1PlayerState a, b;
    sa1_player_reset(&a, sa3ds_movement_lab_world());
    sa1_player_reset(&b, sa3ds_movement_lab_world());
    for (int i = 0; i < 300; ++i) {
        SA1Input scripted = {0};
        scripted.move_z = i < 180 ? 1.0f : 0.0f;
        scripted.jump_pressed = i == 30;
        scripted.jump_held = i >= 30 && i < 42;
        sa1_physics_step(&a, &scripted, 0.0f, sa3ds_movement_lab_world(), &SA1_PHYSICS_PLACEHOLDER_CONSTANTS);
        sa1_physics_step(&b, &scripted, 0.0f, sa3ds_movement_lab_world(), &SA1_PHYSICS_PLACEHOLDER_CONSTANTS);
    }
    assert(memcmp(&a, &b, sizeof(a)) == 0);

    printf("portable physics tests: PASS (placeholder expectations)\n");
    return 0;
}
