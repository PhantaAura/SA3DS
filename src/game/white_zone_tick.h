#ifndef SA3DS_WHITE_ZONE_TICK_H
#define SA3DS_WHITE_ZONE_TICK_H

#include <stdint.h>
#include "../sa1/physics/input.h"

/* A host-testable 60 Hz scheduling boundary, independent of GPU frames.
 * A millisecond clock advances 60 fractional units per millisecond; one
 * physics tick costs 1000 units. No floating point drift or wall-clock
 * changes enter the physics kernel. This is a PORT scheduling fixture,
 * not a claim of measured retail frame pacing. */
#define SA3DS_WZ_TICKS_PER_SECOND 60u
#define SA3DS_WZ_PHASE_PER_TICK 1000u
#define SA3DS_WZ_MAX_STEPS_PER_FRAME 4u
#define SA3DS_WZ_MAX_ELAPSED_MS 250u

typedef struct {
    uint32_t phase;
    uint64_t total_ticks;
    uint64_t dropped_ticks;
    SA1Input pending_input;
    int pending_edges;
} SA3DSWhiteZoneTick;

void sa3ds_wz_tick_reset(SA3DSWhiteZoneTick *clock);
/* Called exactly once per rendered frame, with elapsed wall time and the
 * sampled logical input. Edges persist until a simulation tick consumes them.
 * Returns 0..4 simulation ticks; excess catch-up ticks are counted/dropped. */
unsigned sa3ds_wz_tick_begin(SA3DSWhiteZoneTick *clock,
                             uint32_t elapsed_ms, const SA1Input *input);
/* Returns the input for one of the steps reported by begin(). Edge events
 * are emitted only on the first step; analog/held states repeat. */
SA1Input sa3ds_wz_tick_input(SA3DSWhiteZoneTick *clock, unsigned step_index);

#endif
