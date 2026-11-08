#ifndef SA3DS_CAM_TRIGGER_H
#define SA3DS_CAM_TRIGGER_H
#include <stddef.h>
#include <stdint.h>
/*PC2004 CAM record trigger volumes (437FB0/4373E0 -> 4357F0/435840/435AA0).
 *Record bytes: +0 camera mode index, +1 sub-priority, +2 adjust index,
 *+3 shape (0 sphere, 1 gate, 2 box), +4/+6 u16 BAMS rotX/rotY, +8..+10 center,
 *+14/+18/+1C half extents (sphere: +1C is the squared radius).
 *Every test raises the player positions by4. The broad check uses the current
 *task position; gate/box details use task-position history (4419C0) selector1
 *(and selector2 for the gate's previous point).*/
typedef struct {uint8_t mode,priority,adjust,shape;uint16_t rot_x,rot_y;float center[3],extent[3],point[3],view[3];} SA3DSCamRecord; /*point +24..+2C, view +30..+38*/
int sa3ds_cam_record_decode(SA3DSCamRecord *out,const uint8_t record[64]);
/*Decodes a CAMACTn chunk (raw 64-byte CAM rows). Returns0 for a ragged or
 *over-capacity chunk.*/
int sa3ds_cam_records_decode(SA3DSCamRecord *out,uint32_t capacity,const uint8_t *rows,size_t size,uint32_t *count);
/*Returns1 on success; *inside receives the source result.*/
int sa3ds_cam_trigger_inside(const SA3DSCamRecord *record,const float current[3],
 const float history1[3],const float history2[3],double (*sine)(void *context,uint16_t bams),void *context,int *inside);
/*Priority-4 region pushes on the camera eye (437E90 sphere/box, 437F20 gate),
 *no +4 offset. Sphere 4367B0: |eye-center|^2 < +1C (strict) puts the eye at
 *center + unit*+14. Box 436AD0: inside the inclusive local box, the axis with
 *the smallest gap (ties x, then y) is set to +-extent (<=0 -> negative).
 *Gate 436850: within sqrt(ex^2+ey^2) of center, not moving closer to +24
 *point, and the old->eye segment straddling local z=0: eye = old and the
 *adjust hook gets 436560(0). Its plane constant 915440 is all zeros in the
 *shipped exe, so the 788640 hit is NaN and the rectangle test always passes;
 *the port keeps that. Each returns1 when it moved the eye.*/
int sa3ds_cam_region_sphere_push(const SA3DSCamRecord *record,float eye[3]);
int sa3ds_cam_region_box_push(const SA3DSCamRecord *record,float eye[3],double (*sine)(void *context,uint16_t bams),void *context);
int sa3ds_cam_region_gate_push(const SA3DSCamRecord *record,const float old[3],float eye[3],
 double (*sine)(void *context,uint16_t bams),void *context);
/*Whole 437E90 + 437F20 pass over records count-1..1 (record0 skipped) whose
 *mode has priority4 (mode table +4). Returns the number of pushes; *gate_reverts
 *counts gate reverts (each one is a 436560(0) request).*/
int sa3ds_cam_regions_apply(const SA3DSCamRecord *records,uint32_t count,const uint8_t mode_priority[256],
 const float old[3],float eye[3],double (*sine)(void *context,uint16_t bams),void *context,unsigned *gate_reverts);
#endif
