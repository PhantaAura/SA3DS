#ifndef SA3DS_CAM_ADJUST_H
#define SA3DS_CAM_ADJUST_H
#include "cam_mode4.h"
/*PC2004 camera adjust 469D10 (adjusts 6..26), run by 437A20 after the mode
 *and 435C70 on Camera_Data1, with the previous frame's camera as "old".
 *Table 0x7E0888 (idx-6): {final-ray flag, duration, frame split}. t =
 *counter/duration eases position (4684E0 for idx<=15, 468650 above) and
 *rotations (4686F0) from old toward new, clamps rotations around the old
 *camera's angles to the player (468790), and odd entries run 462CF0. t>=1
 *ends the blend: adjust becomes 0 (0xB in mode 0x4C). Adjust 0 (467D80) is a
 *no-op; adjusts 1-5 and 27 are not ported.*/
typedef struct {
 uint32_t counter;      /*03B2CA34 (work+0x14)*/
 uint8_t use_ray;       /*3C4AC9C*/
 float duration;        /*3C4AC60*/
 int32_t frames;        /*3C4ABD8*/
 int32_t start_x,start_y; /*3C4AC38/AC3C*/
 float t;               /*3C4ABCC*/
} SA3DSCamAdjust;
/*Returns1 when handled (including adjust0), 0 for unported indices or bad
 *input. *adjust_index may become 0/0xB when the blend ends; the counter is
 *advanced like 437B6A.*/
int sa3ds_cam_adjust_run(SA3DSCamera *now,const SA3DSCamera *old,SA3DSCamAdjust *adjust,uint8_t *adjust_index,uint8_t mode,
 const float player[3],const float player_previous[3],int turning,const SA3DSCamServices *services);
#endif
