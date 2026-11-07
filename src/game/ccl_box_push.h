#ifndef SA3DS_CCL_BOX_PUSH_H
#define SA3DS_CCL_BOX_PUSH_H
#include "checkpoint_contact.h"
/*419C90 responses for a primary form7 sweep against a form3 box after the
 *418D80 push predicate passes. Top: taskY=top-.01, rotX/Z=0, status
 *(&~8)|0x20. Underside: taskY=bottom+.01-2*halfHeight, status(&~8)|0x10.
 *Side: 419B50 clamps the sample in box-yaw local XZ to just outside the
 *expanded face (float .999; +X uses radius*.999+halfX), writes world X/Z.
 *Each response is followed by441A40(player,0,taskPosition).*/
typedef struct {
 SA1Vec3 position;     /*player task position after the response*/
 uint16_t status_set,status_clear;
 uint8_t moved,zero_tilt;
} SA3DSBoxPushResult;
/*Pure 419B50 math: sample is the hit sample (task XZ, center Y).*/
int sa3ds_ccl_box_side_push(const SA3DSCheckpointSweep *box,SA1Vec3 sample,float *out_x,float *out_z);
/*Apply the response for an already detected hit when push is enabled.*/
int sa3ds_ccl_box_push(const SA3DSCheckpointSweep *box,const SA3DSCheckpointContact *hit,
 SA1Vec3 task_position,SA3DSBoxPushResult *out);
/*418D80 from the box's side: the player receives the box's push byte.*/
int sa3ds_ccl_box_push_enabled(const SA3DSCCLBody *player,const SA3DSCCLShape *primary,
 const SA3DSCCLBody *box_body,const SA3DSCCLShape *box);
#endif
