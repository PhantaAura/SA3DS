#ifndef SA3DS_WCA_REPLAY_H
#define SA3DS_WCA_REPLAY_H
#include "../sa1/physics/input.h"

/* Developer validation only. Screen-space inputs enter the ordinary native
 * controller. Scenario resets are explicit test setup, never stage completion.
 * No player velocity, action, animation or event flags are injected. */
#define SA3DS_WCA_DIRECTION_CASES 144u
#define SA3DS_WCA_DIRECTION_TICKS 48u
#define SA3DS_WCA_TRANSITION_CASES 8u
#define SA3DS_WCA_TRANSITION_TICKS 360u
#define SA3DS_WCA_REPLAY_TICKS (SA3DS_WCA_DIRECTION_CASES*SA3DS_WCA_DIRECTION_TICKS + SA3DS_WCA_TRANSITION_CASES*SA3DS_WCA_TRANSITION_TICKS)

typedef struct {
    SA1Input screen_input;
    float camera_yaw;
    unsigned case_id;
    unsigned case_tick;
    const char *name;
    int reset;
    int finished;
} SA3DSWCAReplayStep;
SA3DSWCAReplayStep sa3ds_wca_replay_step(unsigned tick);
#endif
