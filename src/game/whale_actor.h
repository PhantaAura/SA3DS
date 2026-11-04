#ifndef SA3DS_WHALE_ACTOR_H
#define SA3DS_WHALE_ACTOR_H
#include "whale_common.h"
#include "whale_tip.h"
/*collision_disabled tracks the SECOND CCI shape attribute0x10, not all CCL.*/
typedef struct {SA3DSWhaleMotion motion;float shared_tip[3];uint8_t collision_disabled;} SA3DSWhaleActor;
typedef struct {
 SA3DSWhaleMotionServices motion;
 SA3DSWhaleCommonServices common;
 /*Dispatch phase requests BEFORE recomputing tip. Destroy is a deferred task
  *request: phase2 still runs the common/display tail; far deletion does not.*/
 void (*phase_requests)(void *context,const SA3DSWhaleMotionResult *requests,const float previous_tip[3]);
 void *phase_context;
} SA3DSWhaleActorServices;
typedef struct {SA3DSWhaleMotionResult phase;SA3DSWhaleCommonResult common;} SA3DSWhaleActorResult;
/*Shared tip survives AO replacement; caller supplies existing stage value.*/
int sa3ds_whale_actor_init(SA3DSWhaleActor *actor,const float position[3],int32_t yaw,
 const float previous_tip[3],const SA3DSWhaleMotionServices *services);
int sa3ds_whale_actor_tick(SA3DSWhaleActor *actor,const SA3DSWhaleTipAssets *tip,
 const SA3DSWhaleMotionInput *input,uint8_t collision_flag8,
 const SA3DSWhaleActorServices *services,SA3DSWhaleActorResult *out);
#endif
