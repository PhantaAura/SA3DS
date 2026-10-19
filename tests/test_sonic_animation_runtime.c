#include <assert.h>
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
    SA3DSSonicAnimationBinding b;
    assert(sa3ds_sonic_animation_binding_count() == 17);
    for (size_t i = 0; i < sa3ds_sonic_animation_binding_count(); ++i) {
        SA3DSSonicAnimationBinding entry, lookup;
        assert(sa3ds_sonic_animation_binding_at(i, &entry));
        assert(sa3ds_sonic_animation_binding(entry.runtime_id, &lookup));
        assert(lookup.action_slot == entry.action_slot);
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

    SA1PlayerState p = base_player();
    SA3DSSonicAnimationSync sync = {0};
    SA3DSSonicAnimationRuntime rt = {0};
    SA3DSSonicAnimationObservation o = observe(&sync, &p);
    SA3DSSonicAnimationSelection s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 0 && s.binding.action_slot == 1);
    assert(s.selector_is_provisional);

    p.sim_frame = 1; p.vel.z = 0.2f;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 9 && s.binding.action_slot == 3);

    p.sim_frame = 2; p.vel.z = SA1_STEAM_SONIC_PARAMETER_EVIDENCE.jog_speed;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 10 && s.binding.action_slot == 4);

    p.sim_frame = 3; p.vel.z = SA1_STEAM_SONIC_PARAMETER_EVIDENCE.run_speed;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 12 && s.binding.action_slot == 6);

    p.sim_frame = 4; p.grounded = false; p.action_state = SA1_ACTION_AIRBORNE;
    p.ball_form = true; p.vel.y = 1.0f;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 14 && s.binding.action_slot == 14);

    p.sim_frame = 5; p.grounded = true; p.action_state = SA1_ACTION_GROUNDED;
    p.ball_form = false; p.vel.y = 0.0f;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 19 && s.binding.action_slot == 11);
    assert(s.ticks_in_selection == 0);

    p.sim_frame = 6; p.action_state = SA1_ACTION_SPINDASH_CHARGE; p.ball_form = true;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 32 && s.binding.action_slot == 21);

    p.sim_frame = 7; p.action_state = SA1_ACTION_ROLL;
    o = observe(&sync, &p); s = select_no_mutation(&rt, &p, &o);
    assert(s.binding.runtime_id == 15 && s.binding.action_slot == 14);

    puts("physics-synced Sonic runtime animation resolver: PASS");
    return 0;
}
