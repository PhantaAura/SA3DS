#include "set_task_lifetime.h"
#include "whale_lifetime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {SA3DSSetTaskLifetime *entry;SA3DSWhaleShared *world;char order[16];unsigned used;uint32_t next;float position[3];int32_t yaw;} Context;
static void event(Context*c,char v){c->order[c->used++]=v;c->order[c->used]=0;}
static void destructor(void*v,uint32_t task){Context*c=v;event(c,'D');assert(c->entry->task==task&&(c->entry->flags&1));c->entry->flags|=4;}
static void defer(void*v,uint32_t task){Context*c=v;event(c,'F');assert(task==c->world->current_task&&c->world->kill_request==1);}
static uint32_t allocate(void*v){Context*c=v;event(c,'A');assert(c->world->kill_request==0);return c->next;}
static void data(void*v,uint32_t task,const float p[3],int32_t yaw){Context*c=v;event(c,'S');assert(c->world->current_task==task);memcpy(c->position,p,12);c->yaw=yaw;}
int main(void){
 SA3DSSetTaskLifetime e={.flags=0x8018,.loads=255};Context c={.entry=&e};uint8_t freed;
 assert(sa3ds_set_task_load_eligible(&e)&&sa3ds_set_task_load_commit(&e,11,160000));
 assert(e.flags==0x8019&&e.loads==0&&e.unload_distance_squared==160100&&!sa3ds_set_task_load_eligible(&e));
 assert(!sa3ds_set_task_load_commit(&e,12,160000));
 assert(sa3ds_set_task_defer_free(&e,11)&&e.task==11&&(e.flags&1)&&!c.used);
 assert(sa3ds_set_task_run_free(&e,11,0,destructor,&c,&freed)&&!freed&&!c.used&&e.task==11);
 assert(sa3ds_set_task_run_free(&e,11,1,destructor,&c,&freed)&&freed&&!strcmp(c.order,"D")&&e.flags==0x801c&&!e.task&&sa3ds_set_task_load_eligible(&e));
 assert(sa3ds_set_task_load_commit(&e,12,160000)&&e.loads==1&&e.flags==0x801d);
 e.flags=0;assert(!sa3ds_set_task_load_eligible(&e));
 SA3DSWhaleShared w={.current_task=21,.kill_request=1,.tip={9,8,7}};c=(Context){.world=&w,.next=22};
 SA3DSWhaleLifetimeServices s={&c,defer,allocate,data};float p[3]={1,2,3};
 assert(sa3ds_whale_spawn(&w,p,-32768,&s)&&!strcmp(c.order,"FAS")&&w.current_task==22&&!w.kill_request&&w.tip[0]==9&&c.yaw==-32768&&!memcmp(c.position,p,12));
 assert(!sa3ds_whale_shared_destruct(&w,21)&&w.current_task==22&&w.tip[0]==9); /*Old task cannot clobber replacement.*/
 assert(sa3ds_whale_kill_request(&w)&&w.kill_request==1);
 assert(sa3ds_whale_shared_destruct(&w,22)&&!w.current_task&&w.tip[0]==0&&w.tip[1]==0&&w.tip[2]==0&&w.kill_request==1);
 assert(!sa3ds_whale_kill_request(&w)&&w.kill_request==1);
 c.used=0;c.order[0]=0;c.next=0;assert(!sa3ds_whale_spawn(&w,p,0,&s)&&!strcmp(c.order,"A")&&!w.current_task&&!w.kill_request);
 puts("SET/whale lifetime: PASS deferred free/destructor ordering,registration guard,active-bit clearing/persistent flags,load counter wrap,reload,old whale replacement identity,kill gating and explicit allocation failure; native scheduler/rumble separate");
}
