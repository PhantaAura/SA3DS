#ifndef SA1_PHYSICS_STEAM_BALL_H
#define SA1_PHYSICS_STEAM_BALL_H

#include "movement.h"
#include "steam_input.h"
#include "steam_player_parameters.h"
#include "steam_spindash_evidence.h"

/*
 * 0.0.8.8.6 ball/Spin Dash parity pass.
 *
 * The ordinary Sonic Spin Dash entry/charge/release/timeout rules below are
 * verified against the exact supported SADX 2004 US executable.  The Steam
 * build remains a comparison source, but its compiler-transformed Sonic state
 * routine has not yet been independently isolated.  Generic state/input
 * architecture remains source-independent.
 */

typedef struct {
    float charge_before;
    float charge_after;
    float forward_speed_before;
    float forward_speed_after;
    unsigned turn_step_bams;
    int entered_charge;
    int charged_this_tick;
    int released_to_roll;
    int timed_out_charge;
    int exited_roll;
    int used_shortcut_request;
} SA1SteamBallDebug;

/* Returns non-zero when the current flat-ground tick was consumed by a
 * Spin Dash / roll state. ACTION and the restored D-down shortcut feed this
 * same state machine and therefore have identical eligibility and physics. */
int sa1_steam_sonic_ball_ground_step(SA1PlayerState *player,
                                     const SA1Input *input,
                                     float camera_yaw,
                                     SA1SteamBallDebug *debug_out);

#endif
