#ifndef SA3DS_RING_REPLAY_H
#define SA3DS_RING_REPLAY_H
#include "ring_runtime.h"
#include "../sa1/physics/input.h"
/* Explicit optional opening-ring validation. Input only: no position, velocity,
 * collision result, animation, ring counter or stage event injection. */
SA1Input sa3ds_ring_replay_input(const SA3DSRings *rings,const SA1PlayerState *player,
    float camera_yaw,unsigned tick);
#endif
