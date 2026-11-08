#ifndef SA3DS_CAM_MODE4_H
#define SA3DS_CAM_MODE4_H
#include <stdint.h>
#include "cam_trigger.h"
/*PC2004 default follow camera: mode4 (469590) on the parameter block, then
 *435C70 apply (camera position = eye, yaw/pitch from eye->target). Modes8-11
 *share 469590 but their placement/sidestep branch is not ported here.*/
typedef struct {
 float eye[3],target[3];      /*parameter block +0C..+14, +18..+20*/
 int32_t roll;                /*+2C, written by apply as camera rotZ*/
} SA3DSCamParam;
typedef struct {
 SA3DSCamParam param;
 float position[3];           /*Camera_Data1 +20..+28*/
 int32_t rot_x,rot_y,rot_z;   /*Camera_Data1 +14/+18/+1C*/
} SA3DSCamera;
typedef struct {
 float player[3],velocity[3]; /*task position, motionwk world velocity*/
 float eyes_height;           /*CharObj2+0x118 (physics eyes_height)*/
 uint32_t frames_since_reset; /*03B2CAEC: 0 re-seeds the target*/
 uint8_t turn_left,turn_right;/*03B0E808/80A analog above0x80*/
 uint8_t manual_disabled;     /*level9 act3*/
} SA3DSCamInput;
typedef struct {
 void *context;
 double (*sine)(void *context,uint16_t bams);
 /*4370C0/43A060: sweep radius sphere from start by delta; on hit modify both
  *(clipped start, sliding delta) and return1.*/
 int (*ray)(void *context,float start[3],float delta[3],float radius);
 /*437E90/437F20 priority-4 CAM region pushes on the eye (may be NULL).*/
 void (*regions)(void *context,float eye[3],const float previous[3]);
} SA3DSCamServices;
int sa3ds_cam_mode4_update(SA3DSCamera *camera,const SA3DSCamInput *input,const SA3DSCamServices *services);
/*Modes20-23 (46A350): fixed eye at the CAM record's +30 point, target =
 *player + (0,7,0). Frames==0 clears manual; with manual set it runs mode4
 *and drops manual (requesting adjust 0x1A) once neither L nor R is held and
 *the player is not standing still (436690: moved <0.1 while grounded).
 *L/R starts manual when 40EF60 (909FB0, default1) allows it. 4625E0 only acts
 *in level6 act2 and is omitted. Returns1 on success; *adjust_request gets
 *0x1A or -1.*/
int sa3ds_cam_mode20_update(SA3DSCamera *camera,const SA3DSCamInput *input,const float fixed_eye[3],
 uint8_t player_still,uint8_t manual_allowed,uint8_t *manual,const SA3DSCamServices *services,int *adjust_request);
/*Inputs used by modes31/39 and the 4629A0 standing orbit.*/
typedef struct {
 uint8_t analog_l,analog_r;   /*03B0E808/80A raw values*/
 uint8_t stick_active;        /*03B0E80C/80E nonzero*/
 uint8_t buttons_active;      /*held buttons & 0xF0*/
 uint8_t player_still;        /*03B2CAAC (436690)*/
 uint8_t airborne;            /*player status byte4 &3 == 0*/
 uint8_t adjust_index;        /*03B2CAE1*/
 uint8_t manual_allowed;      /*40EF60 (909FB0)*/
} SA3DSCamExtra;
typedef struct {uint8_t manual39,manual31,orbit;float height31,drift31;} SA3DSCamModeState; /*3C4ABF0/ABD4/AC50/ABC8/ADEC*/
/*4629A0: standing orbit about the target by (R-L)<<8 * -0.0056875 BAMS while
 *no stick/buttons and the player stands still; starts on L/R with adjust0;
 *leaving it requests adjust8. Returns1 when it placed the eye.*/
int sa3ds_cam_orbit_update(SA3DSCamera *camera,const SA3DSCamInput *input,const SA3DSCamExtra *extra,uint8_t *orbit,
 const SA3DSCamServices *services,int *adjust_request);
/*Mode39 (469E90): orbit, else target=player+(0,7.5,0), eye=target+unit(player-
 *record point+24)*record+3C; regions; 462B40 only when record word+20==0.*/
int sa3ds_cam_mode39_update(SA3DSCamera *camera,const SA3DSCamInput *input,const SA3DSCamExtra *extra,
 const SA3DSCamRecord *record,SA3DSCamModeState *state,const SA3DSCamServices *services,int *adjust_request);
/*Mode31 (46A0A0): horizontal unit(player-point) * +3C, height H (word+20*0.1
 *or +3C*0.5 at frames0) raised by (L-d)/L*H with L the eye's horizontal
 *distance from the point, airborne drift (word+22==0: -0.2/frame, else *0.9),
 *regions, 462B40.*/
int sa3ds_cam_mode31_update(SA3DSCamera *camera,const SA3DSCamInput *input,const SA3DSCamExtra *extra,
 const SA3DSCamRecord *record,SA3DSCamModeState *state,const SA3DSCamServices *services,int *adjust_request);
/*435C70: position=eye, rot_y=atan2(eye-target XZ), rot_x=atan2(dy,horizontal).*/
void sa3ds_cam_apply_look(SA3DSCamera *camera);
#endif
