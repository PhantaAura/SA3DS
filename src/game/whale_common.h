#ifndef SA3DS_WHALE_COMMON_H
#define SA3DS_WHALE_COMMON_H
#include "whale_motion_phase.h"
/*502754..5029BD. The preceding animated-tip calculation is a separate caller
 *stage: tip is sampled BEFORE this function moves the actor horizontally.*/
typedef struct {
 float tip[3];uint32_t kill_request;uint8_t collision_flag8;
} SA3DSWhaleCommonInput;
typedef struct {
 void *context;SA3DSWhaleSine sine;
 /*4419C0: player0/history offset0, never current task-position fallback.*/
 int (*buffered_position)(void *context,float position[3]);
 /*441AD0 returns a CHARACTER id, not a player index; -1 means unrecognized.*/
 int32_t (*collision_character)(void *context);
 void (*bounce_player0)(void *context,const float velocity[3]);
 void (*enqueue_collision)(void *context,const float center[3]);
 uint16_t (*random15)(void *context);
 void (*wave)(void *context,float x,float z,float zero,float scale,float size);
 void (*display)(void *context);
} SA3DSWhaleCommonServices;
typedef struct {float movement[3],collision_center[3];uint8_t bounced,wave;} SA3DSWhaleCommonResult;
/*Call after a successful phase unless stop_common_tail is set. A phase2 delete
 *request still executes this tail. Host double models bounded x87 expressions;
 *full extended-precision word parity and native service wiring remain open.*/
int sa3ds_whale_common(SA3DSWhaleMotion *motion,const SA3DSWhaleCommonInput *input,
 const SA3DSWhaleCommonServices *services,SA3DSWhaleCommonResult *out);
#endif
