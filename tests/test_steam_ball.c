#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "stage.h"
#include "steam_ball.h"
#include "steam_flat.h"

static int nearf(float a, float b, float eps) {
    return fabsf(a - b) <= eps;
}

static void step_world(SA1PlayerState *p, SA1Input in) {
    sa1_physics_step_steam_sonic_flat(p, &in, 0.0f,
        sa3ds_movement_lab_world(), &SA1_PHYSICS_PLACEHOLDER_CONSTANTS, NULL);
}

static float forward_speed(const SA1PlayerState *p) {
    return p->vel.x * sinf(p->yaw) + p->vel.z * cosf(p->yaw);
}

int main(void) {
    SA1PlayerState p, normal, shortcut;
    SA1SteamBallDebug d;

    /* Spin Dash is a grounded action. The helper must not create one from air. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.grounded = false;
    p.action_state = SA1_ACTION_AIRBORNE;
    SA1Input action_press = {0};
    action_press.action_pressed = true;
    assert(sa1_steam_sonic_ball_ground_step(&p, &action_press, 0.0f, &d) == 0);
    assert(p.action_state == SA1_ACTION_AIRBORNE);
    assert(nearf(p.spindash_charge, 0.0f, 0.000001f));

    /* Normal ACTION and restored D-down are semantic aliases into the exact
       same state machine rather than independent physics shortcuts. */
    sa1_player_reset(&normal, sa3ds_movement_lab_world());
    sa1_player_reset(&shortcut, sa3ds_movement_lab_world());
    SA1Input direct = {0}, restored = {0};
    direct.action_pressed = true;
    restored.spindash_shortcut_pressed = true;
    step_world(&normal, direct);
    step_world(&shortcut, restored);
    assert(normal.action_state == SA1_ACTION_SPINDASH_CHARGE);
    assert(shortcut.action_state == SA1_ACTION_SPINDASH_CHARGE);
    assert(normal.ball_form && shortcut.ball_form);
    assert(nearf(normal.spindash_charge, shortcut.spindash_charge, 0.000001f));
    assert(nearf(normal.spindash_charge, SA1_SPINDASH_INITIAL_MIN_PC2004_EVIDENCE,
                 0.000001f));
    assert(nearf(normal.pos.z, shortcut.pos.z, 0.000001f));

    /* Exact 2004 charge progression compares against 10.0 then adds 0.4.
       Because the stored f32 can sit just below 10 after repeated +0.4 steps,
       the next retail tick can cross the comparison threshold to about 10.4. */
    SA1Input hold = {0}; hold.action_held = true;
    for (int i = 0; i < 40; ++i) step_world(&normal, hold);
    assert(normal.action_state == SA1_ACTION_SPINDASH_CHARGE);
    assert(normal.spindash_ticks == 40u);
    assert(normal.spindash_charge > SA1_SPINDASH_MAX_COMPARE_PC2004_EVIDENCE);
    assert(nearf(normal.spindash_charge, 10.3999987f, 0.00002f));
    float standing_release_charge = normal.spindash_charge;

    /* Release transfers the stored charge into local forward speed and enters
       roll, then clears the transient charge accumulator/timer. */
    SA1Input release = {0}; release.action_released = true;
    step_world(&normal, release);
    assert(normal.action_state == SA1_ACTION_ROLL);
    assert(normal.ball_form);
    assert(nearf(forward_speed(&normal), standing_release_charge, 0.0001f));
    assert(nearf(normal.spindash_charge, 0.0f, 0.000001f));
    assert(normal.spindash_ticks == 0u);

    /* The 2004 entry routine carries forward speed into charge above 2.0. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.vel.z = 3.25f;
    step_world(&p, action_press);
    assert(p.action_state == SA1_ACTION_SPINDASH_CHARGE);
    assert(nearf(p.spindash_charge, 3.25f, 0.00001f));

    /* 10.0 is a retail compare threshold, not a post-add clamp. A moving
       9.9 entry receives one last +0.4 tick and becomes 10.3. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.vel.z = 9.9f;
    step_world(&p, action_press);
    assert(nearf(p.spindash_charge, 9.9f, 0.00001f));
    step_world(&p, hold);
    assert(nearf(p.spindash_charge, 10.3f, 0.00002f));
    step_world(&p, hold);
    assert(nearf(p.spindash_charge, 10.3f, 0.00002f));

    /* Entry itself is not capped at 10: speed above the compare threshold is
       copied directly to SpindashSpeed by the 2004 routine. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.vel.z = 12.0f;
    step_world(&p, action_press);
    assert(nearf(p.spindash_charge, 12.0f, 0.00001f));
    step_world(&p, hold);
    assert(nearf(p.spindash_charge, 12.0f, 0.00001f));

    /* SonicSpinTimer is incremented before the transition check. Frame 300
       times out to ordinary grounded state without another held increment.
       The retail timeout path resets the timer but leaves SpindashSpeed stale;
       the neutral state mirrors that internal detail. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    step_world(&p, action_press);
    for (unsigned i = 0; i < SA1_SPINDASH_TIMEOUT_FRAMES_PC2004_EVIDENCE - 1u; ++i)
        step_world(&p, hold);
    assert(p.action_state == SA1_ACTION_SPINDASH_CHARGE);
    assert(p.spindash_ticks == SA1_SPINDASH_TIMEOUT_FRAMES_PC2004_EVIDENCE - 1u);
    float timeout_charge = p.spindash_charge;
    step_world(&p, hold);
    assert(p.action_state == SA1_ACTION_GROUNDED);
    assert(!p.ball_form);
    assert(p.spindash_ticks == 0u);
    assert(nearf(p.spindash_charge, timeout_charge, 0.000001f));

    /* Flat roll uses RollDecel/air_resist and eventually reaches the public
       RollEnd/run_speed threshold, leaving ball form without snapping speed. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.action_state = SA1_ACTION_ROLL;
    p.ball_form = true;
    p.vel.z = 2.0f;
    SA1Input none = {0};
    float before = forward_speed(&p);
    step_world(&p, none);
    assert(p.action_state == SA1_ACTION_ROLL);
    assert(forward_speed(&p) < before);
    for (int i = 0; i < 120 && p.action_state == SA1_ACTION_ROLL; ++i)
        step_world(&p, none);
    assert(p.action_state == SA1_ACTION_GROUNDED);
    assert(!p.ball_form);

    /* Jumping from roll goes through the already-evidenced normal jump impulse
       instead of inventing a second ball-jump formula. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    p.action_state = SA1_ACTION_ROLL;
    p.ball_form = true;
    p.vel.z = 3.0f;
    SA1Input jump = {0}; jump.jump_pressed = true; jump.jump_held = true;
    step_world(&p, jump);
    assert(!p.grounded);
    assert(p.action_state == SA1_ACTION_AIRBORNE);
    assert(p.ball_form);
    assert(nearf(p.vel.y, SA1_STEAM_SONIC_PARAMETER_EVIDENCE.jmp_y_spd -
                          SA1_STEAM_SONIC_PARAMETER_EVIDENCE.weight +
                          SA1_STEAM_SONIC_PARAMETER_EVIDENCE.air_resist_y *
                          SA1_STEAM_SONIC_PARAMETER_EVIDENCE.jmp_y_spd,
                 0.00001f));

    printf("steam roll/spindash PC-2004 evidence tests: PASS\n");
    return 0;
}
