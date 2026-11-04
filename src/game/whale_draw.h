#ifndef SA3DS_WHALE_DRAW_H
#define SA3DS_WHALE_DRAW_H
#include "whale_tip.h"
#include "sa3c_pose.h"
/*Ordinary cycle root motion404DC0/404C10, duration32.*/
int sa3ds_whale_cycle_pitch(const SA3DSWhaleTipAssets *assets,float frame,int32_t *pitch);
/*Action2 selects the owner's null-channel exit motion:404E26 falls back to
 *original root angles (zero in supported v2 profile). V1 cannot authorize
 *that fallback. This models the stock initial descriptor, not guest writes.*/
int sa3ds_whale_root_pitch(const SA3DSWhaleTipAssets *assets,uint8_t action,float frame,int32_t *pitch);
/*502210 order: translation(baseY+drawY), yawY, rollZ, then skeletal root.
 *Shape bank is action2?exit:cycle; caller samples the matching root separately.*/
int sa3ds_whale_draw_transform(const SA3DSWhaleMotion *motion,int32_t root_pitch,
 const SA3DSWhaleMotionServices *services,SA3CMatrix *out,unsigned *shape_bank);
#endif
