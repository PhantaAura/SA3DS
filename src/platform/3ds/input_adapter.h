#ifndef SA3DS_PLATFORM_INPUT_ADAPTER_H
#define SA3DS_PLATFORM_INPUT_ADAPTER_H

#include <3ds.h>
#include "../../sa1/physics/input.h"

/*
 * Development control profile:
 *   Circle Pad  -> movement
 *   A           -> JUMP
 *   B or X      -> ACTION
 *   D-Down      -> RESTORED Spin Dash request
 *   D-Up        -> RESTORED Light Speed request
 *
 * D-Left / D-Right are presentation-side camera snaps in runtime/main.c.
 * L/R camera, Y recenter, debug and pause/exit remain runtime concerns.
 *
 * Device response/deadzone remain provisional until physical Circle Pad
 * calibration is measured. Native traces retain the raw sample and this
 * adapter's normalization separately from the PC2004 filter.
 */
SA1Input sa3ds_read_logical_input(u32 keys_down, u32 keys_held, u32 keys_up);

#endif
