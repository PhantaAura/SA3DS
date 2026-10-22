#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "stage.h"
#include "steam_air.h"
#include "steam_flat.h"
#include "steam_input.h"

static int nearf(float a, float b, float eps) {
    return fabsf(a - b) <= eps;
}

static void step_world(SA1PlayerState *p, SA1Input in) {
    sa1_physics_step_steam_sonic_flat(p, &in, 0.0f,
        sa3ds_movement_lab_world(), &SA1_PHYSICS_PLACEHOLDER_CONSTANTS, NULL);
}

int main(void) {
    const SA1SteamPlayerParameters *sp = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1PlayerState p;
    SA1SteamAirDebug d;
    SA1Input none = {0};

    /* Exact jump-init write identified in the supported Steam EXE. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    sa1_steam_sonic_begin_jump(&p);
    assert(!p.grounded);
    assert(p.action_state == SA1_ACTION_AIRBORNE);
    assert(nearf(p.vel.y, sp->jmp_y_spd, 0.000001f));
    assert(p.jump_ticks == 0);

    /* 0x004DBA90 + Sonic jump state: jmp_addit * 0.8 while held and the
       pre-increment timer remains below jump2_timer. */
    float before = p.vel.y;
    assert(sa1_steam_sonic_apply_jump_hold(&p, 1) == 1);
    assert(p.jump_ticks == 1);
    assert(nearf(p.vel.y - before, sp->jmp_addit * 0.800000011920929f, 0.000001f));
    p.jump_ticks = (unsigned)sp->jump2_timer;
    before = p.vel.y;
    assert(sa1_steam_sonic_apply_jump_hold(&p, 1) == 0);
    assert(p.jump_ticks == (unsigned)sp->jump2_timer + 1u);
    assert(nearf(p.vel.y, before, 0.000001f));

    /* No-input air acceleration is Gravity*weight + the three verified air
       resistance terms. There is no old 0.56 multiplicative jump cut. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.grounded = false;
    p.action_state = SA1_ACTION_AIRBORNE;
    p.vel = (SA1Vec3){0.0f, sp->jmp_y_spd, 0.0f};
    sa1_steam_sonic_air_step(&p, &none, 0.0f, &d);
    assert(nearf(d.accel_vertical,
                 -sp->weight + sp->air_resist_y * sp->jmp_y_spd,
                 0.000001f));
    assert(nearf(p.vel.y, sp->jmp_y_spd + d.accel_vertical, 0.000001f));

    /* Forward air input uses air_accel while rising. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.grounded = false;
    p.vel.y = 0.5f;
    SA1Input fwd = {0}; fwd.move_z = 1.0f;
    SA1SADXFilteredAnalog full = sa1_sadx_filter_logical_analog(0.0f, 1.0f);
    sa1_steam_sonic_air_step(&p, &fwd, 0.0f, &d);
    assert(d.had_analog == 1);
    assert(d.doubled_air_accel == 0);
    assert(nearf(d.requested_force, sp->air_accel * full.magnitude, 0.000001f));

    /* The same small-angle air input doubles its acceleration once vertical
       movement direction is non-positive. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.grounded = false;
    p.vel.y = -0.1f;
    sa1_steam_sonic_air_step(&p, &fwd, 0.0f, &d);
    assert(d.doubled_air_accel == 1);
    assert(nearf(d.requested_force, sp->air_accel * full.magnitude * 2.0f,
                 0.000001f));

    /* Wider than 22.5 degrees: rotate toward input but supply no thrust. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.grounded = false;
    p.vel.y = 0.5f;
    SA1Input right = {0}; right.move_x = 1.0f;
    sa1_steam_sonic_air_step(&p, &right, 0.0f, &d);
    assert(d.blocked_wide_angle_accel == 1);
    assert(nearf(d.requested_force, 0.0f, 0.000001f));
    assert(d.angle_delta_bams == 0x4000u);
    assert(d.turn_step_bams == 0x0800u);

    /* PC2004 0x443CCE..0x443D23: status & 3 == 0 blends the
       old-local vector by explicit f32 0.1 and transformed vector by 0.9.
       The airborne branch does not use the grounded 0.99 coefficient. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.grounded = false;
    p.vel = (SA1Vec3){0.0f, 0.5f, 4.0f};
    sa1_steam_sonic_air_step(&p, &right, 0.0f, &d);
    float turn = 3.14159265358979323846f / 16.0f;
    float f = 0.9f * (4.0f * cosf(turn)) + 0.1f * 4.0f;
    float side = 0.9f * (-4.0f * sinf(turn));
    f += sp->air_resist_air * 4.0f;
    assert(nearf(p.yaw, turn, 0.000001f));
    assert(nearf(p.vel.x, sinf(turn)*f + cosf(turn)*side, 0.000002f));
    assert(nearf(p.vel.z, cosf(turn)*f - sinf(turn)*side, 0.000002f));

    /* High-speed >135-degree opposing input uses air_break. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.grounded = false;
    p.vel = (SA1Vec3){0.0f, 0.5f, 2.0f};
    SA1Input back = {0}; back.move_z = -1.0f;
    SA1SADXFilteredAnalog full_back = sa1_sadx_filter_logical_analog(0.0f, -1.0f);
    sa1_steam_sonic_air_step(&p, &back, 0.0f, &d);
    assert(d.used_air_brake == 1);
    assert(nearf(d.requested_force, sp->air_break * full_back.magnitude, 0.000001f));

    /* End-to-end candidate: holding jump produces a higher apex than tapping. */
    SA1PlayerState tap, hold;
    sa1_player_reset(&tap, sa3ds_movement_lab_world());
    sa1_player_reset(&hold, sa3ds_movement_lab_world());
    for (int frame = 0; frame < 180; ++frame) {
        SA1Input ti = {0}, hi = {0};
        ti.jump_pressed = frame == 0;
        ti.jump_held = frame == 0;
        hi.jump_pressed = frame == 0;
        hi.jump_held = frame < 12;
        step_world(&tap, ti);
        step_world(&hold, hi);
    }
    assert(hold.peak_y > tap.peak_y + 0.25f);

    printf("steam air-parity candidate tests: PASS\n");
    return 0;
}
