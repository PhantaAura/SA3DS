#ifndef SA3DS_WHALE_DRAW_H
#define SA3DS_WHALE_DRAW_H
#include "whale_tip.h"
#include "sa3c_pose.h"
/*Ordinary cycle root motion404DC0/404C10, duration32. Exit's different
 *source motion remains a separate caller-resolved root pitch.*/
int sa3ds_whale_cycle_pitch(const SA3DSWhaleTipAssets *assets,float frame,int32_t *pitch);
/*502210 order: translation(baseY+drawY), yawY, rollZ, then skeletal root.
 *Shape bank is action2?exit:cycle; caller samples the matching root separately.*/
int sa3ds_whale_draw_transform(const SA3DSWhaleMotion *motion,int32_t root_pitch,
 const SA3DSWhaleMotionServices *services,SA3CMatrix *out,unsigned *shape_bank);
#endif
