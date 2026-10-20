#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "stage.h"
#include "steam_flat.h"
#include "steam_input.h"

static void step(SA1PlayerState *p, SA1Input in, SA1SteamFlatDebug *dbg) {
    sa1_physics_step_steam_sonic_flat(p, &in, 0.0f,
        sa3ds_movement_lab_world(), &SA1_PHYSICS_PLACEHOLDER_CONSTANTS, dbg);
}

static float expected_slow_step(float speed) {
    if (speed < SA1_STEAM_SONIC_PARAMETER_EVIDENCE.dash_speed) return 256.0f;
    return 256.0f - lroundf(sqrtf((speed - SA1_STEAM_SONIC_PARAMETER_EVIDENCE.dash_speed) / 16.0f) * 256.0f);
}

int main(void) {
    SA1PlayerState p;
    SA1SteamFlatDebug d;

    /* Steam input shaping: per-component deadzone followed by cubic radius. */
    SA1SADXFilteredAnalog tiny = sa1_sadx_filter_logical_analog(0.05f, 0.0f);
    assert(tiny.active == 0);

    const float half_post_deadzone = (3072.0f + 29441.0f * 0.5f) / 32512.0f;
    SA1SADXFilteredAnalog half = sa1_sadx_filter_logical_analog(0.0f, half_post_deadzone);
    assert(half.active == 1);
    assert(fabsf(half.magnitude - 0.125f) < 0.00002f);

    SA1SADXFilteredAnalog full = sa1_sadx_filter_logical_analog(0.0f, 1.0f);
    assert(full.active == 1);
    assert(full.magnitude > 0.9998f && full.magnitude < 1.0f);

    sa1_player_reset(&p, sa3ds_movement_lab_world());
    SA1Input fwd = {0}; fwd.move_z = 1.0f;
    step(&p, fwd, &d);
    assert(p.animation_accel_residual_valid);
    assert(p.animation_accel_residual == 0.0f);
    assert(d.had_analog == 1);
    assert(fabsf(d.filtered_input_magnitude - full.magnitude) < 0.000001f);
    assert(fabsf(d.requested_force - full.magnitude * 0.05f) < 0.000001f);
    assert(fabsf(d.accel_forward - d.requested_force) < 0.000001f);
    assert(fabsf(d.forward_speed_after - d.accel_forward) < 0.000001f);
    assert(d.angle_delta_bams == 0);

    /* Partial Circle Pad deflection remains partial after PC2004's cubic
       input curve; it must not become a full-strength acceleration request. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    SA1Input half_fwd = {0}; half_fwd.move_z = half_post_deadzone;
    step(&p, half_fwd, &d);
    assert(fabsf(d.filtered_input_magnitude - 0.125f) < 0.00002f);
    assert(fabsf(d.requested_force - 0.125f * 0.05f) < 0.000002f);
    assert(d.requested_force < full.magnitude * 0.05f);

    /* At rest, a 90-degree request uses the 45-degree quick-turn helper and
       contributes no forward force that frame. PC2004 0x443DF0 preserves the
       world-space inertia by transforming it into the new local frame. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.vel.x = 1.0f; /* zero forward speed, nonzero side inertia */
    SA1Input right = {0}; right.move_x = 1.0f;
    step(&p, right, &d);
    assert(d.turn_kind == SA1_STEAM_TURN_QUICK);
    assert(d.turn_step_bams == 0x2000u);
    assert(fabsf(d.requested_force) < 0.000001f);
    assert(d.angle_delta_bams == 0x4000u);
    assert(fabsf(d.forward_speed_after - 0.70710677f) < 0.000002f);
    assert(fabsf(d.side_speed_after - 0.42426407f) < 0.000002f);

    /* Opposite input at speed enters slowdown/braking rather than immediate
       reverse acceleration. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    for (int i = 0; i < 24; ++i) step(&p, fwd, 0);
    SA1Input back = {0}; back.move_z = -1.0f;
    step(&p, back, &d);
    assert(d.used_brake_branch == 1);
    assert(fabsf(d.requested_force - (-0.06f)) < 0.000001f);

    /* PGetSpeed-style integration clamps a forward-speed sign crossing. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.vel.z = 0.03f;
    SA1Input none = {0};
    step(&p, none, &d);
    assert(d.crossed_zero == 1);
    assert(fabsf(d.forward_speed_after) < 0.000001f);

    /* PC2004 0x443C50 retains 99% of pre-turn local velocity and mixes 1% of
       the transformed velocity. This is intentionally not the inverse blend. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.vel.z = 1.0f;
    step(&p, right, &d);
    assert(d.turn_kind == SA1_STEAM_TURN_GENERAL);
    assert(d.turn_step_bams == 0x0800u);
    assert(fabsf(p.yaw) > 0.19f && fabsf(p.yaw) < 0.20f); /* 11.25 degrees */
    const float turned_forward = cosf(p.yaw);
    const float turned_side = -sinf(p.yaw);
    const float blended_forward = 0.99f + 0.01f * turned_forward;
    const float blended_side = 0.01f * turned_side;
    assert(fabsf(d.forward_speed_after - (blended_forward - 0.06f)) < 0.000002f);
    assert(fabsf(d.side_speed_after -
                 (blended_side + blended_side * -0.4f)) < 0.000002f);

    /* PC2004 PGetSpeed 0x44403A..0x4440E1 stops damped local-Z inertia
       when it crosses below 0.87*jog_speed. The old port kept decaying it. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.vel.x = 1.0f;
    step(&p, none, &d);
    assert(fabsf(d.side_speed_before - 1.0f) < 0.000001f);
    assert(fabsf(d.accel_side - (-0.4f)) < 0.000001f);
    assert(fabsf(d.side_speed_after - 0.6f) < 0.000001f);
    assert(d.side_stopped_at_jog_threshold == 0);
    step(&p, none, &d);
    assert(fabsf(d.side_speed_before - 0.6f) < 0.000001f);
    assert(fabsf(d.accel_side - (-0.24f)) < 0.000001f);
    assert(fabsf(d.side_speed_after) < 0.000001f);
    assert(d.side_stopped_at_jog_threshold == 1);

    /* High-speed shallow steering selects Steam's shrinking slow-turn step. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.vel.z = 6.0f;
    SA1Input shallow = {0}; shallow.move_x = 0.10f; shallow.move_z = 1.0f;
    step(&p, shallow, &d);
    assert(d.turn_kind == SA1_STEAM_TURN_SLOW);
    assert((int)d.turn_step_bams == (int)expected_slow_step(6.0f));
    assert(d.turn_step_bams < 0x100u);

    printf("steam ground-parity candidate tests: PASS\n");
    return 0;
}
