#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../src/game/sonic_animation_runtime.h"
#include "../src/sa1/physics/steam_player_parameters.h"

static SA1PlayerState base_player(void) {
    SA1PlayerState p;
    memset(&p, 0, sizeof(p));
    p.grounded = true;
    p.action_state = SA1_ACTION_GROUNDED;
    p.ground_normal = (SA1Vec3){0,1,0};
    p.surface_forward = (SA1Vec3){0,0,1};
    p.surface_right = (SA1Vec3){1,0,0};
    p.animation_accel_residual_valid = true;
    return p;
}

static SA3DSSonicAnimationObservation observe(SA3DSSonicAnimationSync *sync,
                                               SA1PlayerState *p) {
    SA1PlayerState before = *p;
    SA3DSSonicAnimationObservation o = sa3ds_sonic_animation_observe(
        sync, p, &SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
    assert(memcmp(&before, p, sizeof(*p)) == 0);
    return o;
}

static SA3DSSonicAnimationSelection select_no_mutation(
    SA3DSSonicAnimationRuntime *rt, SA1PlayerState *p,
    SA3DSSonicAnimationObservation *o) {
    SA1PlayerState before = *p;
    SA3DSSonicAnimationSelection s = sa3ds_sonic_animation_select(rt, p, o);
    assert(memcmp(&before, p, sizeof(*p)) == 0);
    return s;
}

int main(void) {
    static const struct {
        uint16_t id, slot, next;
        uint8_t instance;
        int8_t property;
        float transition, speed;
    } expected[] = {
        { 0,  1,  0, 73,  3, 0.0625f, 0.5f},
        { 9,  3,  9, 73, 10, 0.25f,   3.4f},
        {10,  4, 10, 73, 10, 0.25f,   1.4f},
        {11,  5, 11, 73, 10, 0.25f,   0.8f},
        {12,  6, 12, 73, 10, 0.25f,   0.5f},
        {13, 18, 13, 73,  9, 0.5f,    0.5f},
        {14, 14, 14, 74, 10, 0.25f,   0.2f},
        {15, 14, 15, 74,  9, 0.25f,   0.7f},
        {16,  8, 16, 73,  3, 0.25f,   1.0f},
        {17,  9, 18, 73,  4, 0.125f,  1.0f},
        {18, 10, 18, 73,  3, 0.25f,   1.0f},
        {19, 11,  0, 73,  4, 1.0f,    1.0f},
        {20,  7,  0, 73,  4, 0.25f,   1.0f},
        {29, 22,  1, 73,  4, 0.0625f, 1.0f},
        {30, 20,  0, 73,  4, 1.0f,    0.9f},
        {31, 19, 31, 73,  3, 0.0625f, 1.0f},
        {32, 21, 32,  1,  3, 0.5f,    1.0f},
    };
    SA3DSSonicAnimationBinding b;
    assert(sa3ds_sonic_animation_binding_count() == sizeof(expected) / sizeof(expected[0]));
    for (size_t i = 0; i < sa3ds_sonic_animation_binding_count(); ++i) {
        SA3DSSonicAnimationBinding entry, lookup;
        assert(sa3ds_sonic_animation_binding_at(i, &entry));
        assert(sa3ds_sonic_animation_binding(entry.runtime_id, &lookup));
        assert(lookup.action_slot == entry.action_slot);
        assert(lookup.instance == entry.instance);
        assert(lookup.property == entry.property);
        assert(lookup.next_runtime_id == entry.next_runtime_id);
        assert(lookup.transition_speed == entry.transition_speed);
        assert(lookup.animation_speed == entry.animation_speed);
        assert(entry.runtime_id == expected[i].id);
        assert(entry.action_slot == expected[i].slot);
        assert(entry.next_runtime_id == expected[i].next);
        assert(entry.instance == expected[i].instance);
        assert(entry.property == expected[i].property);
        assert(entry.transition_speed == expected[i].transition);
        assert(entry.animation_speed == expected[i].speed);
        for (size_t j = 0; j < i; ++j) {
            SA3DSSonicAnimationBinding previous;
            assert(sa3ds_sonic_animation_binding_at(j, &previous));
            assert(previous.runtime_id != entry.runtime_id);
        }
    }
    assert(!sa3ds_sonic_animation_binding_at(sa3ds_sonic_animation_binding_count(), &b));
    assert(sa3ds_sonic_animation_binding(0, &b) && b.action_slot == 1);
    assert(sa3ds_sonic_animation_binding(9, &b) && b.action_slot == 3);
    assert(sa3ds_sonic_animation_binding(10, &b) && b.action_slot == 4);
    assert(sa3ds_sonic_animation_binding(11, &b) && b.action_slot == 5);
    assert(sa3ds_sonic_animation_binding(12, &b) && b.action_slot == 6);
    assert(sa3ds_sonic_animation_binding(13, &b) && b.action_slot == 18);
    assert(sa3ds_sonic_animation_binding(14, &b) && b.action_slot == 14);
    assert(sa3ds_sonic_animation_binding(15, &b) && b.action_slot == 14);
    assert(sa3ds_sonic_animation_binding(18, &b) && b.action_slot == 10);
    assert(sa3ds_sonic_animation_binding(19, &b) && b.action_slot == 11);
    assert(sa3ds_sonic_animation_binding(20, &b) && b.action_slot == 7);
    assert(sa3ds_sonic_animation_binding(32, &b) && b.action_slot == 21);
    assert(!sa3ds_sonic_animation_binding(999, &b));

    assert(sa3ds_sonic_animation_binding(0, &b));
    assert(b.instance == 73 && b.property == 3 && b.next_runtime_id == 0);
    assert(b.transition_speed == 0.0625f && b.animation_speed == 0.5f);
    assert(sa3ds_sonic_animation_binding(9, &b));
    assert(b.instance == 73 && b.property == 10 && b.next_runtime_id == 9);
    assert(fabsf(b.animation_speed - 3.4f) < 1e-6f);
    assert(sa3ds_sonic_animation_binding(18, &b));
    assert(b.property == 3 && b.next_runtime_id == 18 && b.animation_speed == 1.0f);
    assert(sa3ds_sonic_animation_binding(19, &b));
    assert(b.property == 4 && b.next_runtime_id == 0 && b.transition_speed == 1.0f);
    assert(sa3ds_sonic_animation_binding(32, &b));
    assert(b.instance == 1 && b.property == 3 && b.animation_speed == 1.0f);

    SA1PlayerState p = base_player();
    SA3DSSonicAnimationSync sync = {0};
    SA3DSSonicAnimationRuntime rt = {0};
    SA3DSSonicAnimationObservation o = observe(&sync, &p);
    SA3DSSonicAnimationSelection s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 0 && s.binding.action_slot == 1);
    assert(s.selector_is_provisional);
    assert(s.playback_rate_is_source_backed);

    p.sim_frame = 1; p.vel.z = 0.2f;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 9 && s.binding.action_slot == 3);
    assert(s.playback_rate_is_source_backed);

    p.sim_frame = 2; p.vel.z = SA1_STEAM_SONIC_PARAMETER_EVIDENCE.jog_speed;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 10 && s.binding.action_slot == 4);
    assert(s.playback_rate_is_source_backed);

    p.sim_frame = 3; p.vel.z = SA1_STEAM_SONIC_PARAMETER_EVIDENCE.run_speed;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 12 && s.binding.action_slot == 6);

    p.sim_frame = 4; p.grounded = false; p.action_state = SA1_ACTION_AIRBORNE;
    p.ball_form = true; p.vel.y = 1.0f;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 14 && s.binding.action_slot == 14);
    assert(s.playback_rate_is_source_backed);

    p.sim_frame = 5; p.grounded = true; p.action_state = SA1_ACTION_GROUNDED;
    p.ball_form = false; p.vel.y = 0.0f;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 19 && s.binding.action_slot == 11);
    assert(s.ticks_in_selection == 0);
    assert(s.playback_rate_is_source_backed);

    p.sim_frame = 6; p.action_state = SA1_ACTION_SPINDASH_CHARGE; p.ball_form = true;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 32 && s.binding.action_slot == 21);
    assert(s.playback_rate_is_source_backed);

    p.sim_frame = 7; p.action_state = SA1_ACTION_ROLL;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 15 && s.binding.action_slot == 14);
    assert(s.playback_rate_is_source_backed);

    /* Exact PlayAnimation property arithmetic. Property 11 deliberately
     * ignores the row AnimationSpeed. These fixtures exercise the recovered
     * equations; they are not presented as captured retail movement values. */
    SA3DSSonicAnimationSpeedInputs speed_inputs = {2.0f, 3.0f};
    float step_rate = 0.0f;
    b = (SA3DSSonicAnimationBinding){.property=9, .animation_speed=0.5f};
    assert(sa3ds_sonic_animation_frame_step(&b, &speed_inputs, &step_rate));
    assert(fabsf(step_rate - 1.0f) < 1e-6f);
    speed_inputs.forward_speed = -2.0f;
    assert(sa3ds_sonic_animation_frame_step(&b, &speed_inputs, &step_rate));
    assert(fabsf(step_rate + 1.0f) < 1e-6f);
    speed_inputs.forward_speed = 2.0f;
    b.property = 10;
    assert(sa3ds_sonic_animation_frame_step(&b, &speed_inputs, &step_rate));
    assert(fabsf(step_rate - 1.5f) < 1e-6f);
    b.property = 11;
    b.animation_speed = 99.0f;
    assert(sa3ds_sonic_animation_frame_step(&b, &speed_inputs, &step_rate));
    assert(fabsf(step_rate - 3.0f) < 1e-6f);

    /* A changing property-10 speed is accumulated one tick at a time. */
    memset(&sync, 0, sizeof(sync));
    memset(&rt, 0, sizeof(rt));
    p = base_player();
    p.vel.z = 0.2f;
    p.sim_frame = 200;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 9 && s.sample_frame == 0.0f);
    p.vel.z = 0.3f;
    p.sim_frame = 201;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(fabsf(s.sample_frame - 1.02f) < 1e-6f);
    p.animation_accel_residual = 0.01f;
    p.sim_frame = 202;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(fabsf(s.sample_frame - 2.72f) < 1e-5f);

    /* Property 10/11 require the recovered residual's provenance. Unknown
     * surface paths retain the established 0.5 fallback and say so. */
    p.animation_accel_residual_valid = false;
    p.sim_frame = 203;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(!s.playback_rate_is_source_backed);
    assert(fabsf(s.sample_frame - 3.22f) < 1e-5f);

    /* The retail jump initializer sets persistent CharObj2.SpindashSpeed=5;
     * Jump/Spin's property 10 and 0.2 row speed therefore advance by 1.0 even
     * when airborne forward velocity changes on the next tick. */
    memset(&sync, 0, sizeof(sync));
    memset(&rt, 0, sizeof(rt));
    p = base_player();
    p.grounded = false;
    p.action_state = SA1_ACTION_AIRBORNE;
    p.ball_form = true;
    p.vel.z = 0.25f;
    p.sim_frame = 300;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 14 && s.sample_frame == 0.0f);
    p.vel.z = 1.25f;
    p.sim_frame = 301;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 14 && s.playback_rate_is_source_backed);
    assert(fabsf(s.sample_frame - 1.0f) < 1e-6f);

    /* Fixed property 3 is now advanced by the exact retail AnimData speed.
     * Fall is 1.0 frame/tick; the prior provisional global rate was 0.5. */
    memset(&sync, 0, sizeof(sync));
    memset(&rt, 0, sizeof(rt));
    p = base_player();
    p.grounded = false;
    p.action_state = SA1_ACTION_AIRBORNE;
    p.sim_frame = 100;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 18 && s.sample_frame == 0.0f);
    assert(s.playback_rate_is_source_backed);
    p.sim_frame = 101;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 18 && s.sample_frame == 1.0f);

    puts("physics-synced Sonic runtime animation resolver + PC2004 rates: PASS");
    return 0;
}
