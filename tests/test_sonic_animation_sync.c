#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../src/game/sonic_animation_sync.h"

static SA1PlayerState base_player(void) {
    SA1PlayerState p;
    memset(&p, 0, sizeof(p));
    p.grounded = true;
    p.action_state = SA1_ACTION_GROUNDED;
    p.ground_normal = (SA1Vec3){0.0f, 1.0f, 0.0f};
    p.surface_forward = (SA1Vec3){0.0f, 0.0f, 1.0f};
    p.surface_right = (SA1Vec3){1.0f, 0.0f, 0.0f};
    return p;
}

static SA3DSSonicAnimationObservation observe(SA3DSSonicAnimationSync *s,
                                               SA1PlayerState *p) {
    SA1PlayerState before = *p;
    SA3DSSonicAnimationObservation o = sa3ds_sonic_animation_observe(
        s, p, &SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
    assert(memcmp(&before, p, sizeof(*p)) == 0); /* presentation must not mutate physics */
    return o;
}

int main(void) {
    SA3DSSonicAnimationSync s = {0};
    SA1PlayerState p = base_player();

    SA3DSSonicAnimationObservation o = observe(&s, &p);
    assert(o.intent == SA3DS_SONIC_ANIM_GROUNDED_IDLE);
    assert(o.ticks_in_intent == 0);

    p.sim_frame = 1;
    p.vel.z = 0.30f;
    o = observe(&s, &p);
    assert(o.intent == SA3DS_SONIC_ANIM_GROUNDED_BELOW_JOG);
    assert(o.ticks_in_intent == 0);

    p.sim_frame = 2;
    p.vel.z = SA1_STEAM_SONIC_PARAMETER_EVIDENCE.jog_speed;
    o = observe(&s, &p);
    assert(o.intent == SA3DS_SONIC_ANIM_GROUNDED_JOG_TO_RUN);
    assert(o.ticks_in_intent == 0);

    p.sim_frame = 3;
    p.vel.z = SA1_STEAM_SONIC_PARAMETER_EVIDENCE.run_speed;
    o = observe(&s, &p);
    assert(o.intent == SA3DS_SONIC_ANIM_GROUNDED_RUN_PLUS);
    assert(o.ticks_in_intent == 0);
    p.sim_frame = 4;
    o = observe(&s, &p);
    assert(o.ticks_in_intent == 1);

    p.sim_frame = 5;
    p.action_state = SA1_ACTION_SPINDASH_CHARGE;
    p.ball_form = true;
    o = observe(&s, &p);
    assert(o.intent == SA3DS_SONIC_ANIM_SPINDASH_CHARGE && o.ball_form);

    p.sim_frame = 6;
    p.action_state = SA1_ACTION_ROLL;
    o = observe(&s, &p);
    assert(o.intent == SA3DS_SONIC_ANIM_ROLL && o.ticks_in_intent == 0);

    p.sim_frame = 7;
    p.action_state = SA1_ACTION_AIRBORNE;
    p.grounded = false;
    p.ball_form = true;
    p.vel.y = 1.0f;
    o = observe(&s, &p);
    assert(o.intent == SA3DS_SONIC_ANIM_AIRBORNE_BALL);
    assert(o.air_phase == SA3DS_SONIC_AIR_RISING);

    p.sim_frame = 8;
    p.vel.y = 0.0f;
    o = observe(&s, &p);
    assert(o.air_phase == SA3DS_SONIC_AIR_APEX);
    p.sim_frame = 9;
    p.vel.y = -1.0f;
    o = observe(&s, &p);
    assert(o.air_phase == SA3DS_SONIC_AIR_FALLING);

    p.sim_frame = 10;
    p.ball_form = false;
    o = observe(&s, &p);
    assert(o.intent == SA3DS_SONIC_ANIM_AIRBORNE_UPRIGHT);

    puts("sonic animation physics-observer unit tests: PASS");
    return 0;
}
