#ifndef SA3DS_CAM_SELECT_H
#define SA3DS_CAM_SELECT_H
#include "cam_trigger.h"
#include <stdint.h>
/*PC2004 stage CAM selector 437FB0: a per-priority stack of camera states.
 *Records count-1..1 (record0 never) whose mode priority is >= the current
 *priority are visited; 437750 (prio0), 4377A0 (prio1/3) and 437900 (prio2)
 *enter (435DE0), leave (435F50) or pop (436140) states; a pass restarts when
 *the priority falls. 4374C0's idle camera (modes 64/76) is not ported.*/
#define SA3DS_CAM_LEVELS 8u
typedef struct {
 int8_t priority;     /*03B2CAF0[level]*/
 int16_t record;      /*03B2CB08*/
 uint8_t flags;       /*03B2CB14*/
 uint8_t mode;        /*03B2CB50*/
 uint8_t adjust;      /*03B2CB56*/
} SA3DSCamSaved;
typedef struct {
 const uint8_t *priority;      /*mode table +4, 256 entries*/
 const uint8_t *apply;         /*mode table +5, 256 entries*/
 const uint8_t *adjust_flag;   /*adjust table word!=0, 256 entries*/
} SA3DSCamTables;
typedef struct {
 int8_t priority;     /*03B2CAE0*/
 int16_t record;      /*03B2CADC, -1 none*/
 uint8_t mode;        /*03B2CADE*/
 uint8_t adjust;      /*03B2CAE1*/
 uint8_t flags;       /*03B2CAE3 bit0: adjust entry flag*/
 uint8_t apply;       /*03B2CADF*/
 uint8_t subpriority_rose; /*03B2CAC0*/
 uint8_t changed;     /*03B2CAD8*/
 uint8_t reset_frames;/*435F50/436140 clear 03B2CAEC*/
 uint8_t reset_adjust;/*03B2CA34=0*/
 int16_t default_mode;/*03B2CBAC, -1 none*/
 uint8_t default_adjust; /*03B2CAC4*/
 SA3DSCamSaved saved[SA3DS_CAM_LEVELS];
} SA3DSCamSelect;
/*436040(mode,adjust) at priority0; record header byte1 stands in for the
 *sub-priority of "record -1" (the CAM file header precedes the rows).*/
void sa3ds_cam_select_init(SA3DSCamSelect *s,const SA3DSCamTables *t,uint8_t mode,uint8_t adjust);
void sa3ds_cam_select_set(SA3DSCamSelect *s,const SA3DSCamTables *t,const SA3DSCamRecord *records,uint32_t count,uint8_t mode,uint8_t adjust);
/*One 437FB0 pass. current/history1/history2 are the player task position and
 *its 4419C0 history selectors1/2. header_byte1 is CAM file byte1. Returns0 on
 *bad input.*/
int sa3ds_cam_select_update(SA3DSCamSelect *s,const SA3DSCamTables *t,const SA3DSCamRecord *records,uint32_t count,
 uint8_t header_byte1,const float current[3],const float history1[3],const float history2[3],
 double (*sine)(void *context,uint16_t bams),void *context);
#endif
