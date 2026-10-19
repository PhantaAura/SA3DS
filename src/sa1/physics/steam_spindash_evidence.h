#ifndef SA1_PHYSICS_STEAM_SPINDASH_EVIDENCE_H
#define SA1_PHYSICS_STEAM_SPINDASH_EVIDENCE_H

#include <stdint.h>

/*
 * 0.0.8.8.6 SADX 2004 US Spin Dash evidence.
 *
 * These values and state rules are verified directly against the exact
 * supported 2004 US sonic.exe profile (SHA-256 6b0937ca...).  They are no
 * longer working hypotheses.  Note that 10.0f is a comparison threshold, not
 * a hard clamp: the retail routine adds 0.4f when charge is below 10.0f, so a
 * moving entry such as 9.9f can become 10.3f for one final increment.
 */
#define SA1_SPINDASH_INITIAL_MIN_PC2004_EVIDENCE 2.0f
#define SA1_SPINDASH_MAX_COMPARE_PC2004_EVIDENCE 10.0f
#define SA1_SPINDASH_INCREMENT_PC2004_EVIDENCE 0.4f
#define SA1_SPINDASH_TIMEOUT_FRAMES_PC2004_EVIDENCE 300u

float sa1_sadx_spindash_initial_charge(float local_forward_speed);
float sa1_sadx_spindash_charge_held_tick(float charge);
int sa1_sadx_spindash_timeout_reached(uint32_t spin_timer);

#endif
