#ifndef SA1_PHYSICS_TRACE_H
#define SA1_PHYSICS_TRACE_H

#include <stdio.h>
#include "input.h"
#include "player_state.h"

#define SA1_TRACE_FORMAT_VERSION 6

/* Stable CSV v6: UTF-8, LF lines, decimal point '.', one row after each tick.
 * v6 appends the general-surface tangent frame/support diagnostics. */
int sa1_trace_write_header(FILE *output);
int sa1_trace_write_frame(FILE *output, const SA1PlayerState *player,
                          const SA1Input *input);

#endif
