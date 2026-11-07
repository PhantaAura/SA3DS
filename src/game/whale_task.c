#include "whale_task.h"
#include <math.h>
#include <string.h>
typedef struct {SA3DSWhaleTaskRuntime *runtime;SA3DSWhaleTaskSlot *slot;SA3DSOwnedTaskList *list;uint32_t task;int error;} WhaleCall;
static SA3DSWhaleTaskSlot *slot_of(SA3DSOwnedTaskList *l,uint32_t id){
 SA3DSOwnedTask *t=sa3ds_owned_task(l,id);return t?t->data:NULL;
}
static void clear_slot(SA3DSWhaleTaskSlot *s){SA3DSWhaleTaskRuntime *owner=s->owner;memset(s,0,sizeof(*s));s->owner=owner;}
static SA3DSWhaleMotionServices motion_services(const SA3DSWhaleTaskRuntime *r){
 return (SA3DSWhaleMotionServices){r->services.context,r->services.sine};
}
/*5022A0: 4F8930 identity-guarded shared clear, then RumbleA(0,0) regardless.*/
static int destruct_whale(SA3DSOwnedTaskList *l,uint32_t id,void *context){
 (void)context;SA3DSWhaleTaskSlot *s=slot_of(l,id);if(!s)return 0;SA3DSWhaleTaskRuntime *r=s->owner;
 (void)sa3ds_whale_shared_destruct(&r->shared,id);
 if(r->services.rumble_stop)r->services.rumble_stop(r->services.context);
 clear_slot(s);r->destructs++;return 1;
}
/*40B570: release reads the destructor installed by5029F0. A task deleted
 *before its constructor ran has none, so the slot is cleared here instead.*/
static int free_whale(SA3DSOwnedTaskList *l,uint32_t id,void *context){
 SA3DSWhaleTaskSlot *s=slot_of(l,id);if(!s||!sa3ds_owned_task_release(l,id,context))return 0;
 if(s->used)clear_slot(s);
 return 1;
}
static void request_delete(SA3DSWhaleTaskSlot *s,SA3DSOwnedTaskList *l,uint32_t id,int *error){
 if(s->deleting)return;
 if(!sa3ds_owned_task_set_main(l,id,free_whale)){if(error)*error=1;return;}
 s->deleting=1;s->owner->deletion_requests++;
}
static double call_sine(void *v,uint16_t a){WhaleCall *c=v;return c->runtime->services.sine(c->runtime->services.context,a);}
static int call_history(void *v,float p[3]){WhaleCall *c=v;return c->runtime->services.buffered_position(c->runtime->services.context,p);}
static int32_t call_character(void *v){
 WhaleCall *c=v;return c->runtime->services.collision_character(c->runtime->services.context,c->slot->ccl.body.cached_other_id);
}
static void call_bounce(void *v,const float p[3]){WhaleCall *c=v;c->runtime->services.bounce_player0(c->runtime->services.context,p);}
static uint16_t call_random(void *v){WhaleCall *c=v;return c->runtime->services.random15(c->runtime->services.context);}
static void call_wave(void *v,float x,float z,float zero,float scale,float size){
 WhaleCall *c=v;c->runtime->services.wave(c->runtime->services.context,x,z,zero,scale,size);
}
/*502906 copies the pre-movement tip into the first CCI; the zero-local second
 *sphere follows the already moved task origin, then41C280 enqueues.*/
static void call_enqueue(void *v,const float center[3]){
 WhaleCall *c=v;SA3DSWhaleMotion *m=&c->slot->actor.motion;
 if(!sa3ds_whale_ccl_publish(&c->slot->ccl,(SA1Vec3){m->position[0],m->position[1],m->position[2]},
    (SA1Vec3){center[0],center[1],center[2]})){c->error=1;return;}
 c->slot->ccl_queued=1;
}
/*502210 display: action2 selects the exit bank/root, otherwise cycle.*/
static void call_display(void *v){
 WhaleCall *c=v;SA3DSWhaleTaskSlot *s=c->slot;SA3DSWhaleMotionServices ms=motion_services(c->runtime);int32_t pitch;
 int ok=sa3ds_whale_root_pitch(c->runtime->tip,s->actor.motion.action,s->actor.motion.frame,&pitch)&&
    sa3ds_whale_draw_transform(&s->actor.motion,pitch,&ms,&s->draw,&s->bank);
 if(!ok){c->error=1;return;}
 s->frame=s->actor.motion.frame;s->displayed=1;
}
static void call_phase(void *v,const SA3DSWhaleMotionResult *requests,const float previous_tip[3]){
 WhaleCall *c=v;
 if(requests->clear_collision_disable)sa3ds_whale_ccl_emerge(&c->slot->ccl);
 if(c->runtime->services.effects)c->runtime->services.effects(c->runtime->services.context,requests,previous_tip);
 /*Deferred: dive deletion still runs this tail; storage is freed later.*/
 if(requests->destroy)request_delete(c->slot,c->list,c->task,&c->error);
}
static int main_whale(SA3DSOwnedTaskList *l,uint32_t id,void *context){
 (void)context;SA3DSWhaleTaskSlot *s=slot_of(l,id);if(!s||!s->constructed)return 0;
 SA3DSWhaleTaskRuntime *r=s->owner;WhaleCall call={r,s,l,id,0};
 s->ccl_queued=s->displayed=0;
 SA3DSWhaleMotionInput input={.kill_request=r->shared.kill_request};
 if(!r->services.camera_position(r->services.context,input.camera_position))return 0;
 SA3DSWhaleActorServices services={
  motion_services(r),
  {&call,call_sine,call_history,call_character,call_bounce,call_enqueue,call_random,call_wave,call_display},
  call_phase,&call};
 /*Prior CCL status bit8 is consumed before this frame's clear/enqueue.*/
 SA3DSWhaleActorResult result;
 if(!sa3ds_whale_actor_tick(&s->actor,r->tip,&input,(s->ccl.body.status&8)?1:0,&services,&result)||call.error)return 0;
 memcpy(r->shared.tip,s->actor.shared_tip,sizeof(r->shared.tip));
 if(!result.phase.stop_common_tail){r->movement[0]=result.common.movement[0];r->movement[1]=result.common.movement[2];}
 return 1;
}
/*5029F0: actor state, two-sphere CCL with second shape disabled, camera
 *disable, then main/destructor installation for later traversals.*/
static int construct_whale(SA3DSOwnedTaskList *l,uint32_t id,void *context){
 (void)context;SA3DSOwnedTask *t=sa3ds_owned_task(l,id);SA3DSWhaleTaskSlot *s=t?t->data:NULL;
 if(!s||s->constructed)return 0;
 SA3DSWhaleTaskRuntime *r=s->owner;SA3DSWhaleMotionServices ms=motion_services(r);
 if(!sa3ds_whale_actor_init(&s->actor,s->spawn_position,s->spawn_yaw,r->shared.tip,&ms))return 0;
 const float *p=s->actor.motion.position;
 if(!sa3ds_whale_ccl_init(&s->ccl,r->ccl,r->ccl_id_base+id,(SA1Vec3){p[0],p[1],p[2]}))return 0;
 if(r->services.camera_disable)r->services.camera_disable(r->services.context);
 t->destructor=destruct_whale;s->constructed=1;r->constructs++;
 return sa3ds_owned_task_set_main(l,id,main_whale);
}
static void lifetime_defer(void *v,uint32_t task){
 SA3DSWhaleTaskRuntime *r=v;SA3DSWhaleTaskSlot *s=slot_of(r->tasks,task);
 if(s)request_delete(s,r->tasks,task,NULL);
}
static uint32_t lifetime_allocate(void *v){
 SA3DSWhaleTaskRuntime *r=v;
 for(unsigned i=0;i<SA3DS_WHALE_TASK_SLOTS;i++){
  SA3DSWhaleTaskSlot *s=&r->slots[i];if(s->used)continue;
  uint32_t id=sa3ds_owned_task_allocate(r->tasks,construct_whale,s);
  if(!id)break;
  clear_slot(s);s->used=1;s->task=id;return id;
 }
 r->allocation_failures++;return 0;
}
static void lifetime_spawn_data(void *v,uint32_t task,const float p[3],int32_t yaw){
 SA3DSWhaleTaskRuntime *r=v;SA3DSWhaleTaskSlot *s=slot_of(r->tasks,task);
 if(s){memcpy(s->spawn_position,p,sizeof(s->spawn_position));s->spawn_yaw=yaw;}
}
int sa3ds_whale_tasks_init(SA3DSWhaleTaskRuntime *r,SA3DSOwnedTaskList *l,const SA3DSWhaleTipAssets *tip,
 const SA3DSWhaleCCLAssets *ccl,const SA3DSWhaleTaskServices *s){
 if(!r||!l||!l->slots||!tip||!tip->pitch||!ccl||!s||!s->sine||!s->camera_position||!s->buffered_position||
    !s->collision_character||!s->bounce_player0||!s->random15||!s->wave)return 0;
 memset(r,0,sizeof(*r));r->tasks=l;r->tip=tip;r->ccl=ccl;r->services=*s;
 for(unsigned i=0;i<SA3DS_WHALE_TASK_SLOTS;i++)r->slots[i].owner=r;
 return 1;
}
int sa3ds_whale_tasks_summon(SA3DSWhaleTaskRuntime *r,const float p[3],int32_t yaw){
 if(!r||!r->tasks)return 0;
 SA3DSWhaleLifetimeServices s={r,lifetime_defer,lifetime_allocate,lifetime_spawn_data};
 if(!sa3ds_whale_spawn(&r->shared,p,yaw,&s))return 0;
 r->spawns++;return 1;
}
int sa3ds_whale_tasks_kill(SA3DSWhaleTaskRuntime *r){return r&&sa3ds_whale_kill_request(&r->shared);}
SA3DSWhaleTaskSlot *sa3ds_whale_tasks_slot(SA3DSWhaleTaskRuntime *r,unsigned i){
 return r&&i<SA3DS_WHALE_TASK_SLOTS&&r->slots[i].used&&r->slots[i].constructed?&r->slots[i]:NULL;
}
