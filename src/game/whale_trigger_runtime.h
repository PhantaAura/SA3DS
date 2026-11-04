#ifndef SA3DS_WHALE_TRIGGER_RUNTIME_H
#define SA3DS_WHALE_TRIGGER_RUNTIME_H
#include "owned_task_list.h"
#include "set_task_lifetime.h"
#include "set_object_clip.h"
#include "whale_trigger_assets.h"
#include "whale_trigger.h"
typedef struct {
 uint32_t act;unsigned clip_level;uint8_t debug_mode;
 SA3DSSetClipInput clip;
 SA3DSWhaleContactPlayer players[2];
} SA3DSWhaleTriggerFrame;
typedef struct {
 void *context;
 int (*read_frame)(void *context,SA3DSWhaleTriggerFrame *frame);
 int (*summon)(void *context,const float position[3],int32_t yaw);
 void (*kill)(void *context);
 void (*debug_display)(void *context,const SA3DSWhalePlacement *placement);
} SA3DSWhaleTriggerServices;
typedef struct SA3DSWhaleTriggerRuntime SA3DSWhaleTriggerRuntime;
typedef struct {
 SA3DSWhaleTriggerRuntime *owner;uint32_t row;
 SA3DSSetTaskLifetime lifetime;SA3DSWhaleTrigger trigger;
} SA3DSWhaleTriggerEntry;
struct SA3DSWhaleTriggerRuntime {
 const SA3DSWhaleTriggerAssets *assets;SA3DSOwnedTaskList *tasks;
 SA3DSWhaleTriggerEntry *entries;SA3DSWhaleTriggerServices services;
 uint32_t allocation_failures;
};
/*Ordinary source67/68 profile. Caller retains all buffers/views and supplies a
 *shared list3 scheduler and actual SET activation/persistent flags; actor tasks
 *may be created on that same list. This layer does not mark entries active.*/
int sa3ds_whale_triggers_init(SA3DSWhaleTriggerRuntime *runtime,const SA3DSWhaleTriggerAssets *assets,
 SA3DSOwnedTaskList *tasks,SA3DSWhaleTriggerEntry *entries,const uint16_t *initial_flags,
 uint32_t capacity,const SA3DSWhaleTriggerServices *services);
/*Call at the source SET loading phase, then dispatch tasks separately. Normal
 *Sonic EC uses player0 load reference, camera if absent. Special modes/restore
 *snapshots, native stage clearing and debug edits remain caller-owned.*/
int sa3ds_whale_triggers_load(SA3DSWhaleTriggerRuntime *runtime);
#endif
