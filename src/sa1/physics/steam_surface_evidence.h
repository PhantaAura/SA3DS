#ifndef SA1_PHYSICS_STEAM_SURFACE_EVIDENCE_H
#define SA1_PHYSICS_STEAM_SURFACE_EVIDENCE_H

#include "steam_player_parameters.h"

/*
 * 0.0.8.8.7 surface-evidence helpers.
 *
 * These constants/branches are isolated from the exact 2004 PC
 * PGetAcceleration path or from the shared parameter table. They are kept
 * separate from collision/contact code so a public SA1-style collision rule
 * cannot silently become a claim about the supported 2004 executable.
 */

#define SA1_SADX_UNSTICK_SPEED_PC2004_EVIDENCE       1.16f
#define SA1_SADX_UNSTICK_UP_PRIMARY_PC2004_EVIDENCE  0.10f
#define SA1_SADX_UNSTICK_CROSS_PC2004_EVIDENCE       0.60000002f
#define SA1_SADX_UNSTICK_UP_5X_PC2004_EVIDENCE      -0.40f
#define SA1_SADX_UNSTICK_UP_08X_PC2004_EVIDENCE     -0.30f
#define SA1_SADX_UNSTICK_UP_04X_PC2004_EVIDENCE     -0.10f

/* PGetAcceleration-style pre-acceleration scaling after gravity/collision
 * force has been converted into player-local coordinates. out_forward_scale
 * multiplies the local forward term; out_side_scale multiplies local side.
 */
void sa1_sadx_surface_force_scales(float up_y,
                                   float forward_speed,
                                   const SA1SteamPlayerParameters *p,
                                   float *out_forward_scale,
                                   float *out_side_scale);

/* 44C270 normal-force override uses the absolute Y of the cross
 * product between collision-work Up and incoming world motion. */
int sa1_sadx_surface_primary_unstick_branch(float up_y,
                                            float cross_y_abs,
                                            float forward_speed);

/* Additional local-normal acceleration in the fallback branches of
 * 44C270 for sufficiently fast inverted/steep movement. */
float sa1_sadx_surface_unstick_normal_bias(float up_y,
                                           float forward_speed,
                                           float weight);

#endif
