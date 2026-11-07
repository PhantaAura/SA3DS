#ifndef SA3DS_WHALE_TASK_H
#define SA3DS_WHALE_TASK_H
#include "owned_task_list.h"
#include "whale_actor.h"
#include "whale_ccl.h"
#include "whale_draw.h"
#include "whale_lifetime.h"
/*Stage services for the AO task (5029F0 constructor,5022C0 main,502210
 *display,5022A0 destructor). Audio/particles/camera execution stay external:
 *effects receives the exact phase requests and the PREVIOUS shared tip.*/
typedef struct {
 void *context;SA3DSWhaleSine sine;
 int (*camera_position)(void *context,float position[3]);
 int (*buffered_position)(void *context,float position[3]);
 /*441AD0 for the cached CCL partner: character ID, -1 if unrecognized.*/
 int32_t (*collision_character)(void *context,uint32_t other_id);
 void (*bounce_player0)(void *context,const float velocity[3]);
 uint16_t (*random15)(void *context);
 void (*wave)(void *context,float x,float z,float zero,float scale,float size);
 void (*effects)(void *context,const SA3DSWhaleMotionResult *requests,const float previous_tip[3]);
 void (*camera_disable)(void *context);
 void (*rumble_stop)(void *context);
} SA3DSWhaleTaskServices;
struct SA3DSWhaleTaskRuntime;
typedef struct {
 struct SA3DSWhaleTaskRuntime *owner;
 uint32_t task;uint8_t used,constructed,deleting,ccl_queued,displayed;
 float spawn_position[3];int32_t spawn_yaw;
 SA3DSWhaleActor actor;SA3DSWhaleCCL ccl;
 SA3CMatrix draw;unsigned bank;float frame;
} SA3DSWhaleTaskSlot;
#define SA3DS_WHALE_TASK_SLOTS 3
typedef struct SA3DSWhaleTaskRuntime {
 SA3DSOwnedTaskList *tasks;SA3DSWhaleShared shared;
 const SA3DSWhaleTipAssets *tip;const SA3DSWhaleCCLAssets *ccl;
 SA3DSWhaleTaskServices services;SA3DSWhaleTaskSlot slots[SA3DS_WHALE_TASK_SLOTS];
 uint32_t spawns,constructs,destructs,allocation_failures,deletion_requests;
 /*Native CCL actor IDs must not collide with player/NPC IDs; the stage
  *chooses a base added to the owned task identity (0 in isolated tests).*/
 uint32_t ccl_id_base;
 /*5028A1/5028A7 shared movement X/Z globals (03C5E3D8/03C5E3E0), written by
  *every common tail and left in place when no AO task runs.*/
 float movement[2];
} SA3DSWhaleTaskRuntime;
/*Caller keeps tip/CCL assets alive and shares the list3 scheduler with the
 *summon/kill triggers, so a summoned actor waits for a later traversal.*/
int sa3ds_whale_tasks_init(SA3DSWhaleTaskRuntime *runtime,SA3DSOwnedTaskList *tasks,
 const SA3DSWhaleTipAssets *tip,const SA3DSWhaleCCLAssets *ccl,const SA3DSWhaleTaskServices *services);
/*4F88B0 summon service for the trigger runtime.*/
int sa3ds_whale_tasks_summon(SA3DSWhaleTaskRuntime *runtime,const float position[3],int32_t yaw);
/*4F8910 kill service; returns whether a current AO task accepted the request.*/
int sa3ds_whale_tasks_kill(SA3DSWhaleTaskRuntime *runtime);
/*Slot with a published/enqueued CCL this frame, or NULL. Index0..2.*/
SA3DSWhaleTaskSlot *sa3ds_whale_tasks_slot(SA3DSWhaleTaskRuntime *runtime,unsigned index);
#endif
