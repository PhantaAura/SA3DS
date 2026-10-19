/* Presentation timeline regression: same-tick sampling, property-4 landing,
 * rewind, and reset. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/game/sonic_animation_runtime.h"
#include "../src/sa1/physics/steam_player_parameters.h"

static SA1PlayerState player(void) {
    SA1PlayerState p;
    memset(&p, 0, sizeof p);
    p.grounded = true;
    p.action_state = SA1_ACTION_GROUNDED;
    p.ground_normal = (SA1Vec3){0, 1, 0};
    p.surface_forward = (SA1Vec3){0, 0, 1};
    p.surface_right = (SA1Vec3){1, 0, 0};
    return p;
}

static SA3DSSonicAnimationSelection tick(SA3DSSonicAnimationSync *sync,
        SA3DSSonicAnimationRuntime *runtime, SA1PlayerState *p,
        uint16_t expected, uint64_t expected_elapsed) {
    SA1PlayerState unchanged = *p;
    SA3DSSonicAnimationObservation o = sa3ds_sonic_animation_observe(
        sync, p, &SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
    SA3DSSonicAnimationSelection sel = sa3ds_sonic_animation_select(runtime, p, &o);
    assert(sel.binding.runtime_id == expected);
    assert(sel.ticks_in_selection == expected_elapsed);
    if (sel.binding.property == 3 || sel.binding.property == 4)
        assert(sel.playback_rate_is_source_backed);
    /* HUD/render may request the same completed frame multiple times. */
    for (int i=0; i<10; ++i) {
        SA3DSSonicAnimationObservation again = sa3ds_sonic_animation_observe(
            sync, p, &SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
        SA3DSSonicAnimationSelection again_sel =
            sa3ds_sonic_animation_select(runtime, p, &again);
        assert(memcmp(&o, &again, sizeof o) == 0);
        assert(memcmp(&sel, &again_sel, sizeof sel) == 0);
    }
    assert(memcmp(p, &unchanged, sizeof *p) == 0);
    return sel;
}

static void run_landing_sequence(void) {
    SA3DSSonicAnimationSync sync = {0};
    SA3DSSonicAnimationRuntime runtime = {0};
    SA1PlayerState p = player();
    tick(&sync, &runtime, &p, 0, 0);
    p.sim_frame = 1; p.grounded = false;
    p.action_state = SA1_ACTION_AIRBORNE; p.ball_form = true;
    tick(&sync, &runtime, &p, 14, 0);
    p.sim_frame = 2;
    tick(&sync, &runtime, &p, 14, 1);
    p.sim_frame = 3; p.grounded = true;
    p.action_state = SA1_ACTION_GROUNDED; p.ball_form = false;
    SA3DSSonicAnimationSelection landing = tick(&sync, &runtime, &p, 19, 0);
    assert(landing.binding.action_slot == 11);
    /* Action 11 has 16 frames. PlayAnimation advances property 4 until
     * frame >= frames-1 and then installs NextAnim 0 without blending. */
    for (uint64_t frame=4;frame<=17;frame++) {
        p.sim_frame=frame;
        SA3DSSonicAnimationSelection held=tick(&sync,&runtime,&p,19,frame-3);
        assert(held.sample_frame==(float)(frame-3));
    }
    p.sim_frame=18;
    tick(&sync,&runtime,&p,0,0);
    /* Rewind to an earlier physics frame: old transition and elapsed ticks
     * must not survive into the new trace timeline. */
    p.sim_frame = 1; p.grounded = false;
    p.action_state = SA1_ACTION_AIRBORNE; p.ball_form = true;
    tick(&sync, &runtime, &p, 14, 0);
    p.sim_frame = 2; p.grounded = true;
    p.action_state = SA1_ACTION_GROUNDED; p.ball_form = false;
    /* No synthetic landing on a fresh timeline that began airborne, but
     * landing is valid on the following observed airborne->grounded tick. */
    tick(&sync, &runtime, &p, 19, 0);
    /* Explicit reset at same sim_frame must reset both state machines. */
    memset(&sync, 0, sizeof sync);
    memset(&runtime, 0, sizeof runtime);
    tick(&sync, &runtime, &p, 0, 0);
}

int main(void) {
    for (int i=0; i<128; ++i) run_landing_sequence();
    puts("animation same-tick/landing/rewind/reset: PASS (128 deterministic replays)");
    return 0;
}
