#include "whale_trigger_runtime.h"
#include "set_object_clip.h"
#include <string.h>
static int free_trigger(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 SA3DSOwnedTask*t=sa3ds_owned_task(l,id);if(!t||!t->data)return 0;
 SA3DSWhaleTriggerEntry*e=t->data;
 if(!sa3ds_owned_task_release(l,id,context))return 0;
 uint8_t freed;return sa3ds_set_task_run_free(&e->lifetime,id,1,NULL,NULL,&freed)&&freed;
}
static int main_trigger(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 (void)context;SA3DSOwnedTask*t=sa3ds_owned_task(l,id);if(!t||!t->data)return 0;
 SA3DSWhaleTriggerEntry*e=t->data;SA3DSWhaleTriggerRuntime*r=e->owner;
 SA3DSWhaleTriggerFrame f;SA3DSWhalePlacement p;
 if(!r->services.read_frame(r->services.context,&f)||!sa3ds_whale_trigger_placement(r->assets,e->row,&p)||p.act!=f.act)return 0;
 SA3DSSetClipInput clip=f.clip;clip.has_set_data=1;clip.set_flags=(uint8_t)e->lifetime.flags;
 clip.unload_distance_squared=e->lifetime.unload_distance_squared;memcpy(clip.position,p.position,12);
 uint8_t unload;if(!sa3ds_set_object_clip(&clip,&unload))return 0;
 if(unload)return sa3ds_set_task_defer_free(&e->lifetime,id)&&sa3ds_owned_task_set_main(l,id,free_trigger);
 SA3DSWhaleTriggerInput input={.debug_mode=f.debug_mode};
 /*Action1 summon and terminal action2 never invoke441840 again.*/
 if(!f.debug_mode&&e->trigger.action==0&&!sa3ds_whale_trigger_inside(p.position,r->assets->radius,f.players,&input.inside_player))return 0;
 SA3DSWhaleTriggerResult result;SA3DSWhaleTrigger next=e->trigger;
 int ok=(p.type&4095)==67?sa3ds_whale_summon_tick(&next,&input,&result):sa3ds_whale_kill_tick(&next,&input,&result);
 if(!ok)return 0;
 if(result.debug_display){if(!r->services.debug_display)return 0;r->services.debug_display(r->services.context,&p);}
 if(result.summon){float position[3];if(!sa3ds_whale_summon_position(p.position,p.parameters,position)||!r->services.summon(r->services.context,position,p.angles[1]))return 0;}
 if(result.request_kill)r->services.kill(r->services.context);
 e->trigger=next; /*Source stores action2 only after4F88B0 returns.*/
 return 1;
}
static int construct_trigger(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 (void)context;SA3DSOwnedTask*t=sa3ds_owned_task(l,id);if(!t||!t->data)return 0;
 SA3DSWhaleTriggerEntry*e=t->data;e->trigger.action=0;
 return sa3ds_owned_task_set_main(l,id,main_trigger);
}
int sa3ds_whale_triggers_init(SA3DSWhaleTriggerRuntime*r,const SA3DSWhaleTriggerAssets*a,
 SA3DSOwnedTaskList*l,SA3DSWhaleTriggerEntry*entries,const uint16_t*flags,uint32_t capacity,const SA3DSWhaleTriggerServices*s){
 if(!r||!a||!a->rows||a->count>64||!l||!l->slots||!entries||!flags||capacity<a->count||!s||!s->read_frame||!s->summon||!s->kill)return 0;
 memset(r,0,sizeof(*r));r->assets=a;r->tasks=l;r->entries=entries;r->services=*s;
 memset(entries,0,(size_t)a->count*sizeof(*entries));
 for(uint32_t i=0;i<a->count;i++){entries[i].owner=r;entries[i].row=i;entries[i].lifetime.flags=flags[i];}
 return 1;
}
int sa3ds_whale_triggers_load(SA3DSWhaleTriggerRuntime*r){
 if(!r||!r->assets||!r->services.read_frame)return 0;
 SA3DSWhaleTriggerFrame f;if(!r->services.read_frame(r->services.context,&f)||f.act>1||f.clip_level>2)return 0;
 const SA3DSSetClipReference*reference=f.clip.player0.present?&f.clip.player0:&f.clip.camera;
 if(!reference->present)return 1;
 for(uint32_t i=0;i<r->assets->count;i++){
  SA3DSWhaleTriggerEntry*e=&r->entries[i];SA3DSWhalePlacement p;
  if(!sa3ds_whale_trigger_placement(r->assets,i,&p))return 0;
  if(p.act!=f.act||!sa3ds_set_task_load_eligible(&e->lifetime))continue;
  if(e->lifetime.flags&2)return 0; /*Restored SET work needs its actual payload.*/
  unsigned high=(p.type>>12)&7;
  if(high&&(high==1?f.clip_level>=1:f.clip_level>=2))continue;
  int inside;if(!sa3ds_set_position_in_range(p.position,reference->position,r->assets->load_distance_squared,&inside))return 0;
  if(!inside)continue;
  uint32_t id=sa3ds_owned_task_allocate(r->tasks,construct_trigger,e);
  if(!id){r->allocation_failures++;continue;} /*46BBC9 skips failed allocations.*/
  if(!sa3ds_set_task_load_commit(&e->lifetime,id,r->assets->load_distance_squared))return 0;
  e->trigger.action=0;
 }
 return 1;
}
