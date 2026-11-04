#ifndef SA3DS_WHALE_TIP_H
#define SA3DS_WHALE_TIP_H
#include "whale_motion_phase.h"
#include <stddef.h>
typedef struct {const unsigned char *pitch;uint8_t exit_root_bind;} SA3DSWhaleTipAssets;
/*Borrowed owner package view, valid while package bytes remain alive.*/
int sa3ds_whale_tip_assets_open(SA3DSWhaleTipAssets *out,const void *bytes,size_t size);
/*50264A..502754: original pitch interpolation, then Y/X matrix rotations.
 *Accepts frames0..30 inclusive. Exact30 is NOT folded to0. C double preserves
 *observed float stores, but does not establish complete x87 word parity.*/
int sa3ds_whale_tip(const SA3DSWhaleMotion *motion,const SA3DSWhaleTipAssets *assets,
 const SA3DSWhaleMotionServices *services,float out[3],int32_t *sampled_pitch);
#endif
