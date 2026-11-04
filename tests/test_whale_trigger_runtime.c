#include "whale_trigger_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {SA3DSWhaleTriggerFrame frame;SA3DSWhaleTriggerRuntime *runtime;unsigned reads,summons,kills,constructors,mains;float spawn[3];int32_t yaw;} Context;
static int read_frame(void*v,SA3DSWhaleTriggerFrame*f){Context*c=v;c->reads++;*f=c->frame;return 1;}
static int whale_main(SA3DSOwnedTaskList*l,uint32_t id,void*v){(void)l;(void)id;((Context*)v)->mains++;return 1;}
static int whale_constructor(SA3DSOwnedTaskList*l,uint32_t id,void*v){((Context*)v)->constructors++;return sa3ds_owned_task_set_main(l,id,whale_main);}
static int summon(void*v,const float p[3],int32_t yaw){
 Context*c=v;assert(c->runtime->entries[0].trigger.action==1); /*Before source action2 store.*/
 c->summons++;memcpy(c->spawn,p,12);c->yaw=yaw;
 return sa3ds_owned_task_allocate(c->runtime->tasks,whale_constructor,NULL)!=0;
}
static void kill(void*v){((Context*)v)->kills++;}
static void w16(unsigned char*p,uint16_t u){p[0]=(unsigned char)u;p[1]=(unsigned char)(u>>8);}
static void w32(unsigned char*p,uint32_t u){for(unsigned k=0;k<4;k++)p[k]=(unsigned char)(u>>(8*k));}
static void wf(unsigned char*p,float f){uint32_t u;memcpy(&u,&f,4);w32(p,u);}
int main(void){
 unsigned char rows[80]={0};w32(rows+4,5);w16(rows+8,67);w16(rows+12,(uint16_t)-123);
 wf(rows+16,10);wf(rows+20,20);wf(rows+24,30);wf(rows+28,1);wf(rows+32,2);wf(rows+36,3);
 w32(rows+44,8);w16(rows+48,68);wf(rows+56,200);
 SA3DSWhaleTriggerAssets a={rows,2,45,160000};SA3DSOwnedTask slots[8];SA3DSOwnedTaskList list;assert(sa3ds_owned_tasks_init(&list,slots,8));
 SA3DSWhaleTriggerRuntime runtime;SA3DSWhaleTriggerEntry entries[2];Context c={.runtime=&runtime};
 c.frame.clip.player0.present=c.frame.clip.camera.present=1;c.frame.clip_level=1;
 c.frame.players[0]=(SA3DSWhaleContactPlayer){{10,20,30},1,1};
 SA3DSWhaleTriggerServices services={&c,read_frame,summon,kill,NULL};
 const uint16_t flags[2]={0x8000,0x8000};
 assert(sa3ds_whale_triggers_init(&runtime,&a,&list,entries,flags,2,&services)&&sa3ds_whale_triggers_load(&runtime));
 assert(entries[0].lifetime.task&&entries[1].lifetime.task&&list.head==entries[1].lifetime.task);
 unsigned reads=c.reads;assert(sa3ds_owned_tasks_run(&list,&c)&&c.reads==reads&&!entries[0].trigger.action&&!c.summons); /*Constructor only.*/
 assert(sa3ds_owned_tasks_run(&list,&c)&&entries[0].trigger.action==1&&!c.summons);
 assert(sa3ds_owned_tasks_run(&list,&c)&&entries[0].trigger.action==2&&c.summons==1&&!c.constructors&&c.yaw==-123&&c.spawn[0]==11&&c.spawn[1]==22&&c.spawn[2]==33);
 assert(sa3ds_owned_tasks_run(&list,&c)&&c.constructors==1&&!c.mains); /*Actor constructor waits for traversal.*/
 c.frame.players[0].center[0]=200;c.frame.players[0].center[1]=c.frame.players[0].center[2]=0;
 assert(sa3ds_owned_tasks_run(&list,&c)&&c.kills==1&&c.mains==1);
 assert(sa3ds_owned_tasks_run(&list,&c)&&c.kills==2&&c.summons==1);
 c.frame.clip.player0.position[0]=c.frame.clip.camera.position[0]=1000;
 assert(sa3ds_owned_tasks_run(&list,&c)&&entries[0].lifetime.free_main&&entries[1].lifetime.free_main&&entries[0].lifetime.task);
 assert(sa3ds_whale_triggers_load(&runtime)&&entries[0].lifetime.loads==1); /*Pending frees remain active.*/
 assert(sa3ds_owned_tasks_run(&list,&c)&&!entries[0].lifetime.task&&!entries[1].lifetime.task&&entries[0].lifetime.flags==0x8000);
 c.frame.clip.player0.position[0]=c.frame.clip.camera.position[0]=0;
 assert(sa3ds_whale_triggers_load(&runtime)&&entries[0].lifetime.loads==2&&!entries[0].trigger.action);
 assert(sa3ds_owned_tasks_run(&list,&c)&&!entries[0].trigger.action&&c.summons==1);
 SA3DSOwnedTask small[1];SA3DSOwnedTaskList short_list;SA3DSWhaleTriggerRuntime short_runtime;SA3DSWhaleTriggerEntry short_entries[2];
 assert(sa3ds_owned_tasks_init(&short_list,small,1)&&sa3ds_whale_triggers_init(&short_runtime,&a,&short_list,short_entries,flags,2,&services));
 assert(sa3ds_whale_triggers_load(&short_runtime)&&short_runtime.allocation_failures==1&&short_entries[0].lifetime.task&&!short_entries[1].lifetime.task&&short_entries[1].lifetime.loads==0);
 const uint16_t unmarked[2]={0,0};
 assert(sa3ds_owned_tasks_init(&short_list,small,1)&&sa3ds_whale_triggers_init(&short_runtime,&a,&short_list,short_entries,unmarked,2,&services));
 assert(sa3ds_whale_triggers_load(&short_runtime)&&!short_list.head); /*Never invent SET activation.*/
 short_entries[0].lifetime.flags|=0x8000;
 assert(sa3ds_whale_triggers_load(&short_runtime)&&short_entries[0].lifetime.task&&!short_entries[1].lifetime.task);
 puts("Whale trigger runtime: PASS SET head order,constructor/contact/arm/summon timing,post-service action2 store,shared-list actor constructor delay,repeated kill,deferred unload/reload and visible allocation failure count; mock actor services,not native chase");
}
