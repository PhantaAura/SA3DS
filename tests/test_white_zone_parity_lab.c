#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../src/game/parity_lab.h"
#include "../src/sa1/physics/physics_constants.h"
#include "../src/sa1/physics/steam_flat.h"

static void step(SA1PlayerState *p, SA1Input *in) {
    sa1_physics_step_steam_sonic_flat(
        p, in, 0.0f, sa3ds_parity_lab_world(),
        &SA1_PHYSICS_PLACEHOLDER_CONSTANTS, 0);
}

int main(void) {
    size_t count = 0;
    const SA3DSParityZone *zones = sa3ds_parity_lab_zones(&count);
    assert(zones && count == 10u && count == SA3DS_PARITY_ZONE_COUNT);
    assert(fabsf(sa3ds_parity_lab_half_width() - 12.0f) < 1e-6f);
    assert(fabsf(sa3ds_parity_lab_marker_spacing() - 10.0f) < 1e-6f);
    assert(!strcmp(zones[SA3DS_PARITY_ZONE_TURN].name, "TURN"));
    assert(!strcmp(zones[SA3DS_PARITY_ZONE_AIR_LANDING].name, "AIR CONTROL / LANDING"));

    for (size_t i = 0; i < count; ++i) {
        assert(zones[i].name && zones[i].name[0]);
        assert(zones[i].z_end > zones[i].z_start);
        assert(zones[i].reset_z >= zones[i].z_start && zones[i].reset_z < zones[i].z_end);
        if (i) assert(fabsf(zones[i - 1].z_end - zones[i].z_start) < 1e-6f);
        SA1Surface s = sa3ds_parity_lab_surface(0, 0.0f, zones[i].reset_z);
        assert(s.valid && s.surface_id == zones[i].surface_id);
    }

    SA1Surface up = sa3ds_parity_lab_surface(0, 0, 335.0f);
    SA1Surface plateau = sa3ds_parity_lab_surface(0, 0, 370.0f);
    SA1Surface down = sa3ds_parity_lab_surface(0, 0, 405.0f);
    assert(up.valid && up.grad_z > 0.0f && up.ground_y > 0.0f);
    assert(plateau.valid && fabsf(plateau.ground_y - 8.0f) < 1e-6f);
    assert(down.valid && down.grad_z < 0.0f && down.ground_y > 0.0f);

    SA1PlayerState p;
    assert(sa3ds_parity_lab_reset_player(&p, SA3DS_PARITY_ZONE_ACCELERATION));
    assert(p.grounded && p.surface_id == 201 && fabsf(p.pos.z - 24.0f) < 1e-6f);
    assert(fabsf(p.pos.y - 1.0f) < 1e-6f);

    float previous = 0.0f;
    for (unsigned f = 0; f < 30; ++f) {
        SA1Input in = {0}; in.move_z = 1.0f;
        step(&p, &in);
        float speed = sa1_horizontal_speed(&p);
        assert(speed + 1e-6f >= previous);
        previous = speed;
    }
    assert(previous > 0.5f);

    assert(sa3ds_parity_lab_reset_player(&p, SA3DS_PARITY_ZONE_JUMP));
    SA1Input jump = {0}; jump.jump_pressed = true; jump.jump_held = true;
    step(&p, &jump);
    assert(!p.grounded && p.action_state == SA1_ACTION_AIRBORNE && p.ball_form);

    assert(sa3ds_parity_lab_reset_player(&p, SA3DS_PARITY_ZONE_ROLL_SPINDASH));
    SA1Input action = {0}; action.action_pressed = true; action.action_held = true;
    step(&p, &action);
    assert(p.action_state == SA1_ACTION_SPINDASH_CHARGE);
    for (unsigned f = 0; f < 10; ++f) {
        memset(&action, 0, sizeof(action)); action.action_held = true; step(&p, &action);
    }
    memset(&action, 0, sizeof(action)); action.action_released = true; step(&p, &action);
    assert(p.action_state == SA1_ACTION_ROLL && p.ball_form);

    /* 0G: test the real physics out-of-bounds reset path, not just the helper. */
    assert(sa3ds_parity_lab_reset_player(&p, SA3DS_PARITY_ZONE_TURN));
    p.pos.y = sa3ds_parity_lab_world()->reset_min_y - 1.0f;
    SA1Input zero = {0};
    step(&p, &zero);
    assert(fabsf(p.pos.z - zones[SA3DS_PARITY_ZONE_TURN].reset_z) < 1e-6f);
    assert(fabsf(p.pos.x) < 1e-6f);
    assert(p.grounded);

    puts("White Zone parity-lab fixture tests: PASS");
    return 0;
}
