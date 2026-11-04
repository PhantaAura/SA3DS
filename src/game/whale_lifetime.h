#ifndef SA3DS_WHALE_LIFETIME_H
#define SA3DS_WHALE_LIFETIME_H
#include <stdint.h>
typedef struct {uint32_t current_task,kill_request;float tip[3];} SA3DSWhaleShared;
typedef struct {
 void *context;
 void (*defer_free)(void *context,uint32_t task);
 /*Allocate flags3/list3/constructor5029F0. It does NOT execute the constructor
  *inline. The real task allocator/scheduler and resource allocation are external.*/
 uint32_t (*allocate)(void *context);
 void (*set_spawn_data)(void *context,uint32_t task,const float position[3],int32_t yaw);
} SA3DSWhaleLifetimeServices;
/*4F88B0: mark old task,clear kill,allocate,associate new task,write spawn data.
 *The existing tip is retained. Allocation failure is reported, not hidden by
 *inventing a rollback that the source lacks.*/
int sa3ds_whale_spawn(SA3DSWhaleShared *shared,const float position[3],int32_t yaw,
 const SA3DSWhaleLifetimeServices *services);
/*4F8910 only sets shared kill if a current AO task exists.*/
int sa3ds_whale_kill_request(SA3DSWhaleShared *shared);
/*4F8930 called by5022A0: only the matching current task clears pointer/tip.
 *Kill request is not cleared. Caller still requests RumbleA(0,0) even when
 *this helper returns0:5022A0 always does that after4F8930.*/
int sa3ds_whale_shared_destruct(SA3DSWhaleShared *shared,uint32_t task);
#endif
