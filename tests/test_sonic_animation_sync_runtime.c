#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/game/sonic_animation_sync.h"
#include "../src/sa1/physics/movement.h"
#include "../src/sa1/physics/steam_flat.h"
#include "../src/sa1/physics/surface_frame.h"
#include "../src/game/stage.h"

static SA3DSSonicAnimationObservation observe_no_mutation(
    SA3DSSonicAnimationSync *sync, SA1PlayerState *p) {
    SA1PlayerState before = *p;
    SA3DSSonicAnimationObservation o = sa3ds_sonic_animation_observe(
        sync, p, &SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
    assert(memcmp(&before, p, sizeof(*p)) == 0);
    return o;
}

static void step(SA1PlayerState *p, SA1Input *i) {
    sa1_physics_step_steam_sonic_flat(
        p, i, 0.0f, sa3ds_movement_lab_world(),
        &SA1_PHYSICS_PLACEHOLDER_CONSTANTS, NULL);
}

int main(void) {
    SA1PlayerState p;
    SA3DSSonicAnimationSync sync = {0};
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    SA3DSSonicAnimationObservation o = observe_no_mutation(&sync, &p);
    assert(o.intent == SA3DS_SONIC_ANIM_GROUNDED_IDLE);

    /* Physics-backed speed classes switch on the same completed simulation tick
       that tangent speed crosses the evidence parameter. */
    int saw_jog = 0, saw_run = 0;
    for (unsigned f=0; f<240 && !saw_run; ++f) {
        SA1Input in = {0}; in.move_z = 1.0f;
        step(&p, &in);
        float speed = sa1_surface_tangent_speed(&p);
        o = observe_no_mutation(&sync, &p);
        if (!saw_jog && speed >= SA1_STEAM_SONIC_PARAMETER_EVIDENCE.jog_speed) {
            assert(o.intent == SA3DS_SONIC_ANIM_GROUNDED_JOG_TO_RUN ||
                   o.intent == SA3DS_SONIC_ANIM_GROUNDED_RUN_PLUS);
            saw_jog = 1;
        }
        if (!saw_run && speed >= SA1_STEAM_SONIC_PARAMETER_EVIDENCE.run_speed) {
            assert(o.intent == SA3DS_SONIC_ANIM_GROUNDED_RUN_PLUS);
            saw_run = 1;
        }
    }
    assert(saw_jog && saw_run);

    /* Actual Spin Dash physics transition: charge -> release/roll, same tick. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    memset(&sync, 0, sizeof(sync));
    SA1Input in = {0}; in.action_pressed = true; in.action_held = true;
    step(&p, &in);
    o = observe_no_mutation(&sync, &p);
    assert(p.action_state == SA1_ACTION_SPINDASH_CHARGE);
    assert(o.intent == SA3DS_SONIC_ANIM_SPINDASH_CHARGE);

    for (unsigned f=1; f<20; ++f) {
        memset(&in, 0, sizeof(in)); in.action_held = true;
        step(&p, &in);
        o = observe_no_mutation(&sync, &p);
        assert(p.action_state == SA1_ACTION_SPINDASH_CHARGE);
        assert(o.intent == SA3DS_SONIC_ANIM_SPINDASH_CHARGE);
    }
    memset(&in, 0, sizeof(in)); in.action_released = true;
    step(&p, &in);
    o = observe_no_mutation(&sync, &p);
    assert(p.action_state == SA1_ACTION_ROLL);
    assert(o.intent == SA3DS_SONIC_ANIM_ROLL);
    assert(o.ticks_in_intent == 0);

    /* Jump physics enters airborne ball presentation on the same tick. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    memset(&sync, 0, sizeof(sync));
    memset(&in, 0, sizeof(in)); in.jump_pressed = true; in.jump_held = true;
    step(&p, &in);
    o = observe_no_mutation(&sync, &p);
    assert(p.action_state == SA1_ACTION_AIRBORNE);
    assert(p.ball_form);
    assert(o.intent == SA3DS_SONIC_ANIM_AIRBORNE_BALL);
    assert(o.air_phase == SA3DS_SONIC_AIR_RISING);

    /* Loop/wall orientation does not break roll intent: action state owns it. */
    sa1_player_reset(&p, sa3ds_movement_lab_world());
    memset(&sync, 0, sizeof(sync));
    p.pos = (SA1Vec3){0.0f, 18.0f, 238.0f};
    p.vel = (SA1Vec3){0.0f, 2.0f, 0.0f};
    p.grounded = true;
    p.ground_normal = (SA1Vec3){0.0f, 0.0f, -1.0f};
    p.surface_forward = (SA1Vec3){0.0f, 1.0f, 0.0f};
    p.surface_right = (SA1Vec3){1.0f, 0.0f, 0.0f};
    p.action_state = SA1_ACTION_ROLL;
    p.ball_form = true;
    o = observe_no_mutation(&sync, &p);
    assert(o.intent == SA3DS_SONIC_ANIM_ROLL);

    puts("sonic animation/real physics integration tests: PASS");
    return 0;
}
