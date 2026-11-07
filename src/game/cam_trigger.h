#ifndef SA3DS_CAM_TRIGGER_H
#define SA3DS_CAM_TRIGGER_H
#include <stdint.h>
/*PC2004 CAM record trigger volumes (437FB0/4373E0 -> 4357F0/435840/435AA0).
 *Record bytes: +0 camera mode index, +1 sub-priority, +2 adjust index,
 *+3 shape (0 sphere, 1 gate, 2 box), +4/+6 u16 BAMS rotX/rotY, +8..+10 center,
 *+14/+18/+1C half extents (sphere: +1C is the squared radius).
 *Every test raises the player positions by4. The broad check uses the current
 *task position; gate/box details use task-position history (4419C0) selector1
 *(and selector2 for the gate's previous point).*/
typedef struct {uint8_t mode,priority,adjust,shape;uint16_t rot_x,rot_y;float center[3],extent[3];} SA3DSCamRecord;
int sa3ds_cam_record_decode(SA3DSCamRecord *out,const uint8_t record[64]);
/*Returns1 on success; *inside receives the source result.*/
int sa3ds_cam_trigger_inside(const SA3DSCamRecord *record,const float current[3],
 const float history1[3],const float history2[3],double (*sine)(void *context,uint16_t bams),void *context,int *inside);
#endif
