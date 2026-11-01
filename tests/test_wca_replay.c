#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "wca_replay.h"
#define PI 3.14159265358979323846f
int main(void) {
    unsigned resets=0,jumps=0,charges=0,releases=0;
    for(unsigned tick=0;tick<SA3DS_WCA_REPLAY_TICKS;tick++) {
        SA3DSWCAReplayStep step=sa3ds_wca_replay_step(tick);
        assert(!step.finished && step.name);
        assert(step.case_id<152);
        assert(step.reset==(step.case_tick==0));
        assert(!step.screen_input.device_sample_valid);
        assert(isfinite(step.camera_yaw));
        assert(fabsf(step.screen_input.move_x)<=1 && fabsf(step.screen_input.move_z)<=1);
        assert(!step.screen_input.spindash_shortcut_pressed);
        resets+=step.reset;jumps+=step.screen_input.jump_pressed;
        charges+=step.screen_input.action_pressed;releases+=step.screen_input.action_released;
        if(step.case_id<SA3DS_WCA_DIRECTION_CASES) {
            assert(step.case_tick<48);
            assert(fabsf(step.camera_yaw-(step.case_id/18)*PI/4)<1e-6f);
            float length=hypotf(step.screen_input.move_x,step.screen_input.move_z);
            unsigned direction=(step.case_id/2)%9;
            assert(fabsf(length-(direction?(step.case_id%2?1:.5f):0))<1e-6f);
            assert(!step.screen_input.jump_held && !step.screen_input.action_held);
        }else assert(step.case_tick<360);
    }
    assert(resets==152 && jumps==2 && charges==2 && releases==2);
    assert(sa3ds_wca_replay_step(SA3DS_WCA_REPLAY_TICKS).finished);
    assert(sa3ds_wca_replay_step(~0u).finished);
    unsigned start=SA3DS_WCA_DIRECTION_CASES*SA3DS_WCA_DIRECTION_TICKS;
    SA3DSWCAReplayStep jump=sa3ds_wca_replay_step(start+4*360+60);
    assert(jump.screen_input.jump_pressed && jump.screen_input.jump_held);
    SA3DSWCAReplayStep release=sa3ds_wca_replay_step(start+5*360+80);
    assert(release.screen_input.action_released && !release.screen_input.action_held);
    puts("WCA replay plan: PASS (144 circular direction cases;8 controller-transition cases; explicit resets; bounded input-only protocol)");
    return 0;
}
