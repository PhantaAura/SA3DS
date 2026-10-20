#ifndef SA3DS_GAME_PARITY_TRACE_H
#define SA3DS_GAME_PARITY_TRACE_H

#include <stdio.h>

#include "parity_lab.h"
#include "sonic_animation_runtime.h"
#include "../sa1/physics/input.h"
#include "../sa1/physics/player_state.h"
#include "../sa1/physics/steam_flat.h"

#define SA3DS_PARITY_TRACE_VERSION 2

int sa3ds_parity_trace_write_header(FILE *output);
int sa3ds_parity_trace_write_frame(FILE *output,
    SA3DSParityZoneId test_id,
    const char *test_name,
    const SA1PlayerState *player,
    const SA1Input *input,
    float camera_yaw,
    const SA1SteamFlatDebug *physics,
    const SA3DSSonicAnimationSelection *animation);

#endif
