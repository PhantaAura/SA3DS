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
        uint16_t root, frames;
    } expected[] = {
        { 0,  1,  0, 73,  3, 0.0625f, 0.5f, 72,32},
        { 1,122,  2, 73,  4, 0.25f,   0.25f,72,20},
        { 2,123,  2, 73,  3, 0.25f,   0.25f,72,16},
        { 4,120,  5, 73,  4, 0.25f,   0.25f,72,20},
        { 5,121,  6, 73,  4, 0.25f,   0.25f,72,39},
        { 6,120,  0, 73,  6, 0.25f,   0.25f,72,20},
        { 9,  3,  9, 73, 10, 0.25f,   3.4f, 72,32},
        {10,  4, 10, 73, 10, 0.25f,   1.4f, 72,32},
        {11,  5, 11, 73, 10, 0.25f,   0.8f, 72,32},
        {12,  6, 12, 73, 10, 0.25f,   0.5f, 72,32},
        {13, 18, 13, 73,  9, 0.5f,    0.5f, 72,33},
        {14, 14, 14, 74, 10, 0.25f,   0.2f,146, 7},
        {15, 14, 15, 74,  9, 0.25f,   0.7f,146, 7},
        {16,  8, 16, 73,  3, 0.25f,   1.0f, 72,32},
        {17,  9, 18, 73,  4, 0.125f,  1.0f, 72,33},
        {18, 10, 18, 73,  3, 0.25f,   1.0f, 72,32},
        {19, 11,  0, 73,  4, 1.0f,    1.0f, 72,16},
        {20,  7,  0, 73,  4, 0.25f,   1.0f, 72,33},
        {29, 22,  1, 73,  4, 0.0625f, 1.0f, 72,16},
        {30, 20,  0, 73,  4, 1.0f,    0.9f, 72,16},
        {31, 19, 31, 73,  3, 0.0625f, 1.0f, 72,16},
        {32, 21, 32,  1,  3, 0.5f,    1.0f,147, 7},
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
        assert(entry.source_root_node == expected[i].root);
        assert(entry.source_frame_count == expected[i].frames);
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

    /* 0x00495CD0 compares Speed.x + 5*residual against Sonic's exact
     * jog/run/rush/dash thresholds and selects all five ground clips. */
    static const struct { float speed; uint16_t id; } ground_cases[] = {
        {0.20f,9},
        {0.46000000834465027f,9},
        {0.47f,10},
        {1.3899999856948853f,10},
        {1.40f,11},
        {2.299999952316284f,11},
        {2.31f,12},
        {5.090000152587891f,12},
        {5.10f,13},
    };
    for (size_t i=0;i<sizeof(ground_cases)/sizeof(ground_cases[0]);i++) {
        SA3DSSonicAnimationSync case_sync={0};
        SA3DSSonicAnimationRuntime case_rt={0};
        SA1PlayerState case_player=base_player();
        case_player.vel.z=ground_cases[i].speed;
        SA3DSSonicAnimationObservation case_o=observe(&case_sync,&case_player);
        SA3DSSonicAnimationSelection case_s=
            select_no_mutation(&case_rt,&case_player,&case_o);
        assert(case_s.binding.runtime_id==ground_cases[i].id);
    }

    /* 0x494FF0 tests signed local Speed.x against EXACT zero before the
     * landing branch calls 0x495CD0. These are executable-derived selector
     * fixtures, not captured retail terrain traces. */
    for (size_t i=0;i<sizeof(ground_cases)/sizeof(ground_cases[0])+2;i++) {
        float speed = i < sizeof(ground_cases)/sizeof(ground_cases[0])
            ? ground_cases[i].speed : (i == sizeof(ground_cases)/sizeof(ground_cases[0])
                ? -1.0f : 0.000001f);
        uint16_t expected_id = i < sizeof(ground_cases)/sizeof(ground_cases[0])
            ? ground_cases[i].id : 9;
        SA3DSSonicAnimationSync case_sync={0};
        SA3DSSonicAnimationRuntime case_rt={0};
        SA1PlayerState case_player=base_player();
        case_player.grounded=false;case_player.action_state=SA1_ACTION_AIRBORNE;
        SA3DSSonicAnimationObservation case_o=observe(&case_sync,&case_player);
        SA3DSSonicAnimationSelection case_s=
            select_no_mutation(&case_rt,&case_player,&case_o);
        assert(case_s.binding.runtime_id==18);
        case_player.sim_frame=1;case_player.grounded=true;
        case_player.action_state=SA1_ACTION_GROUNDED;case_player.vel.z=speed;
        case_o=observe(&case_sync,&case_player);
        case_s=select_no_mutation(&case_rt,&case_player,&case_o);
        assert(case_s.binding.runtime_id==expected_id);
        assert(case_rt.requested_runtime_id==expected_id);
        assert(case_s.sample_frame==0.0f);
        assert(case_s.transition_active);
        assert(case_s.transition_weight==
            (expected_id==13 ? 0.5f : 0.25f));
        SA3DSSonicAnimationRuntime before=case_rt;
        (void)select_no_mutation(&case_rt,&case_player,&case_o);
        assert(memcmp(&before,&case_rt,sizeof(before))==0);
    }

    /* A provisional stationary Land request is not a property-4 lock.
     * Ordinary upright idle requests 0 (0x4916D7), movement requests its
     * ground clip; both retain the existing destination blend/frame rules. */
    for (int move=0;move<2;move++) {
        SA3DSSonicAnimationSync case_sync={0};
        SA3DSSonicAnimationRuntime case_rt={0};
        SA1PlayerState case_player=base_player();
        case_player.grounded=false;case_player.action_state=SA1_ACTION_AIRBORNE;
        SA3DSSonicAnimationObservation case_o=observe(&case_sync,&case_player);
        (void)select_no_mutation(&case_rt,&case_player,&case_o);
        case_player.sim_frame=1;case_player.grounded=true;
        case_player.action_state=SA1_ACTION_GROUNDED;
        case_o=observe(&case_sync,&case_player);
        SA3DSSonicAnimationSelection case_s=
            select_no_mutation(&case_rt,&case_player,&case_o);
        assert(case_s.binding.runtime_id==19&&!case_s.transition_active);
        case_player.sim_frame=2;case_player.vel.z=move ? 0.2f : 0;
        case_o=observe(&case_sync,&case_player);
        case_s=select_no_mutation(&case_rt,&case_player,&case_o);
        assert(case_s.binding.runtime_id==(move ? 9 : 0));
        assert(case_s.transition_active&&case_s.sample_frame==0);
        assert(case_s.transition_from_action_slot==11);
        int blend_ticks=move ? 3 : 15;
        for(int tick=0;tick<blend_ticks;tick++) {
            case_player.sim_frame++;
            case_o=observe(&case_sync,&case_player);
            case_s=select_no_mutation(&case_rt,&case_player,&case_o);
        }
        assert(!case_s.transition_active&&case_s.sample_frame==0);
        case_player.sim_frame++;
        case_o=observe(&case_sync,&case_player);
        case_s=select_no_mutation(&case_rt,&case_player,&case_o);
        assert(fabsf(case_s.sample_frame-(move ? 0.68f : 0.5f))<1e-6f);
    }
    puts("PC2004 moving landing / idle and movement interruption: PASS");

    /* Same-root Run2 -> Walk3 uses the destination's 0.25 transition row. */
    {
        SA3DSSonicAnimationSync case_sync={0};
        SA3DSSonicAnimationRuntime case_rt={0};
        SA1PlayerState case_player=base_player();
        case_player.vel.z=5.10f;
        SA3DSSonicAnimationObservation case_o=observe(&case_sync,&case_player);
        SA3DSSonicAnimationSelection case_s=
            select_no_mutation(&case_rt,&case_player,&case_o);
        assert(case_s.binding.runtime_id==13&&!case_s.transition_active);
        case_player.sim_frame=1;case_player.vel.z=1.40f;
        case_o=observe(&case_sync,&case_player);
        case_s=select_no_mutation(&case_rt,&case_player,&case_o);
        assert(case_s.binding.runtime_id==11&&case_s.transition_active);
        assert(case_s.transition_from_action_slot==18);
        assert(case_s.transition_weight==0.25f);
    }

    /* Ground -> ball jump and ball jump -> upright fall change ACTION roots,
     * so PlayAnimation explicitly bypasses transition construction. */
    {
        SA3DSSonicAnimationSync case_sync={0};
        SA3DSSonicAnimationRuntime case_rt={0};
        SA1PlayerState case_player=base_player();
        SA3DSSonicAnimationObservation case_o=observe(&case_sync,&case_player);
        (void)select_no_mutation(&case_rt,&case_player,&case_o);
        case_player.sim_frame=1;case_player.grounded=false;
        case_player.action_state=SA1_ACTION_AIRBORNE;case_player.ball_form=true;
        case_o=observe(&case_sync,&case_player);
        SA3DSSonicAnimationSelection case_s=
            select_no_mutation(&case_rt,&case_player,&case_o);
        assert(case_s.binding.runtime_id==14&&!case_s.transition_active);
        case_player.sim_frame=2;case_player.ball_form=false;
        case_o=observe(&case_sync,&case_player);
        case_s=select_no_mutation(&case_rt,&case_player,&case_o);
        assert(case_s.binding.runtime_id==18&&!case_s.transition_active);
    }

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
    assert(s.transition_active && s.transition_from_action_slot == 1);
    assert(s.transition_from_frame == 0.0f && s.transition_weight == 0.25f);
    assert(s.playback_rate_is_source_backed);

    /* A root change bypasses blending even though Jump's row says 0.25. */
    p.sim_frame = 2; p.grounded = false; p.action_state = SA1_ACTION_AIRBORNE;
    p.ball_form = true; p.vel.y = 1.0f;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    /* PlayAnimation does not interrupt an in-flight state-2 blend. */
    assert(s.binding.runtime_id == 9 && s.transition_weight == 0.5f);
    p.sim_frame = 3;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 9 && s.transition_weight == 0.75f);
    p.sim_frame = 4;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 9 && !s.transition_active);
    p.sim_frame = 5;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 14 && !s.transition_active);
    assert(s.playback_rate_is_source_backed);

    p.sim_frame = 6; p.grounded = true; p.action_state = SA1_ACTION_GROUNDED;
    p.ball_form = false; p.vel.y = 0.0f; p.vel.z = 0.0f;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 19 && s.binding.action_slot == 11);
    assert(s.ticks_in_selection == 0);
    assert(s.playback_rate_is_source_backed);

    p.sim_frame = 7; p.action_state = SA1_ACTION_SPINDASH_CHARGE; p.ball_form = true;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 14 && s.binding.action_slot == 14);
    assert(s.playback_rate_is_source_backed);

    p.sim_frame = 8; p.action_state = SA1_ACTION_ROLL;
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

    /* Original roll exit requests 29, retained by ground selectors. Its
     * property-4 chain is 29 -> 1 -> 2, not fallback to unbound idle zero. */
    memset(&sync,0,sizeof(sync)); memset(&rt,0,sizeof(rt));
    p=base_player();p.action_state=SA1_ACTION_ROLL;p.ball_form=true;
    p.vel.z=0.1f;
    o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    assert(s.binding.runtime_id==15);
    p.sim_frame++;p.action_state=SA1_ACTION_GROUNDED;p.ball_form=false;p.vel.z=0;
    o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    assert(s.binding.runtime_id==29&&s.sample_frame==0&&!s.transition_active);
    for(unsigned tick=1;tick<=15;tick++){
        p.sim_frame++;
        o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
        assert(s.binding.runtime_id==(tick<15?29:1));
        assert(s.sample_frame==(tick<15?(float)tick:0.0f));
        SA3DSSonicAnimationSelection again=select_no_mutation(&rt,&p,&o);
        assert(!memcmp(&s,&again,sizeof(s)));
    }
    assert(s.binding.action_slot==122&&!s.transition_active);
    /* Once NextAnim installs 1, moving ground selection is free to replace
     * it; only unchanged idle input retains the 1 -> 2 idle continuation. */
    SA3DSSonicAnimationRuntime moving_rt=rt;
    SA3DSSonicAnimationSync moving_sync=sync;
    SA1PlayerState moving=p;moving.sim_frame++;moving.vel.z=0.5f;
    SA3DSSonicAnimationObservation moving_o=observe(&moving_sync,&moving);
    SA3DSSonicAnimationSelection moving_s=select_no_mutation(&moving_rt,&moving,&moving_o);
    assert(moving_s.binding.runtime_id==10&&moving_s.transition_active);
    for(unsigned tick=1;tick<=76;tick++){
        p.sim_frame++;
        o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
        assert(s.binding.runtime_id==(tick<76?1:2));
        assert(fabsf(s.sample_frame-(tick<76?tick*0.25f:0.0f))<1e-6f);
    }
    p.sim_frame++;o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    assert(s.binding.runtime_id==2&&s.binding.action_slot==123&&s.sample_frame==0.25f);
    p.vel.z=0.5f;p.sim_frame++;
    o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    assert(s.binding.runtime_id==10&&s.transition_active);

    /* A changed action request interrupts a one-shot, but not an in-flight
     * state-2 blend. Exercise charge, jump, and falling after Uncurl entry. */
    for(unsigned next=0;next<3;next++){
        memset(&sync,0,sizeof(sync));memset(&rt,0,sizeof(rt));
        p=base_player();p.action_state=SA1_ACTION_ROLL;p.ball_form=true;
        o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
        p.sim_frame++;p.action_state=SA1_ACTION_GROUNDED;p.ball_form=false;
        o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);assert(s.binding.runtime_id==29);
        p.sim_frame++;p.grounded=next==0;p.ball_form=next!=2;
        p.action_state=next==0?SA1_ACTION_SPINDASH_CHARGE:SA1_ACTION_AIRBORNE;
        p.spindash_charge=2;
        o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
        assert(s.binding.runtime_id==(next==2?18:14));
    }
    puts("PC2004 Uncurl request/idle-chain/action interruptions: PASS");

    /* Supported ordinary static landings retain pre-projection WORLD Y.
     * Ball retail action8 and upright action12 have different soft requests. */
    for(unsigned ball=0;ball<2;ball++)for(unsigned hard=0;hard<3;hard++){
        memset(&sync,0,sizeof(sync));memset(&rt,0,sizeof(rt));
        p=base_player();p.grounded=false;p.action_state=SA1_ACTION_AIRBORNE;
        p.ball_form=ball!=0;p.vel.y=-2;
        o=observe(&sync,&p);(void)select_no_mutation(&rt,&p,&o);
        p.sim_frame=1;p.grounded=true;p.action_state=SA1_ACTION_GROUNDED;
        p.ball_form=false;p.vel.y=0;
        p.animation_landing_valid=true;p.animation_landing_air_action=ball?8:12;
        p.animation_landing_world_y=hard==0?-4.999f:(hard==1?-5.0f:-5.001f);
        o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
        assert(s.binding.runtime_id==(hard?0:(ball?2:1)));
        assert(s.sample_frame==0);
        assert(rt.landing_classification_valid);
        assert(rt.landing_classified_request==(hard?19:(ball?2:1)));
        assert(rt.requested_runtime_id==(hard?0:(ball?2:1)));
        assert(rt.idle_counter==2);
    }
    /* Active processed input at EXACT zero local speed bypasses stationary
     * landing and also starts locomotion from idle; no magnitude is changed. */
    memset(&sync,0,sizeof(sync));memset(&rt,0,sizeof(rt));
    p=base_player();p.grounded=false;p.action_state=SA1_ACTION_AIRBORNE;
    o=observe(&sync,&p);(void)select_no_mutation(&rt,&p,&o);
    p.sim_frame=1;p.grounded=true;p.action_state=SA1_ACTION_GROUNDED;
    p.animation_input_valid=true;p.animation_input_active=true;
    p.animation_landing_valid=true;p.animation_landing_air_action=12;
    p.animation_landing_world_y=-8;
    o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    assert(s.binding.runtime_id==9&&rt.requested_runtime_id==9);

    /* Original signed idle counter compares >1800 BEFORE increment. From
     * zero the 1802nd selector call requests4, not after an invented duration. */
    memset(&sync,0,sizeof(sync));memset(&rt,0,sizeof(rt));p=base_player();
    for(uint64_t tick=0;tick<=1800;tick++){
        p.sim_frame=tick;o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
        assert(s.binding.runtime_id==0&&rt.idle_counter==tick+1);
    }
    p.sim_frame++;o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    assert(s.binding.runtime_id==4&&s.transition_active&&rt.idle_counter==1);
    for(unsigned tick=0;tick<3;tick++){
        p.sim_frame++;o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    }
    assert(!s.transition_active&&s.sample_frame==0);
    for(unsigned tick=0;tick<76;tick++){
        p.sim_frame++;o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    }
    assert(s.binding.runtime_id==5&&s.sample_frame==0&&!s.transition_active);
    for(unsigned tick=0;tick<152;tick++){
        p.sim_frame++;o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    }
    assert(s.binding.runtime_id==6&&s.sample_frame==19&&!s.transition_active);
    for(unsigned tick=0;tick<76;tick++){
        p.sim_frame++;o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    }
    assert(s.binding.runtime_id==6&&s.sample_frame==0);
    p.sim_frame++;o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    assert(s.binding.runtime_id==0&&s.sample_frame==0);
    /* Same-tick reads do not increment counters or consume another request. */
    uint16_t idle_counter=rt.idle_counter;
    (void)select_no_mutation(&rt,&p,&o);assert(rt.idle_counter==idle_counter);
    rt.idle_counter=0xffff;rt.idle_selector_active=1;
    p.sim_frame++;o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    assert(s.binding.runtime_id==0&&rt.idle_counter==0);
    rt.idle_counter=1801;p.ground_normal.y=0.8f;
    p.sim_frame++;o=observe(&sync,&p);s=select_no_mutation(&rt,&p,&o);
    assert(s.binding.runtime_id==0&&rt.idle_counter==1801);
    puts("PC2004 static landing/input + upright idle counter/reverse chain: PASS");
    puts("physics-synced Sonic runtime animation resolver + PC2004 rates: PASS");
    return 0;
}
