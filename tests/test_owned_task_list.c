#include "owned_task_list.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {char order[64];unsigned used;uint32_t spawned,target;int insert,replace,destructor_insert;} Context;
static void event(Context*c,char e){assert(c->used+1<sizeof(c->order));c->order[c->used++]=e;c->order[c->used]=0;}
static int main_actor(SA3DSOwnedTaskList*l,uint32_t id,void*v){(void)l;(void)id;event(v,'M');return 1;}
static int constructor(SA3DSOwnedTaskList*l,uint32_t id,void*v){event(v,'C');return sa3ds_owned_task_set_main(l,id,main_actor);}
static int first(SA3DSOwnedTaskList*l,uint32_t id,void*v){
 (void)id;Context*c=v;event(c,'A');
 if(c->insert){c->spawned=sa3ds_owned_task_allocate(l,constructor,NULL);assert(c->spawned);c->insert=0;}
 if(c->replace){assert(sa3ds_owned_task_set_main(l,c->target,main_actor));c->replace=0;}
 return 1;
}
static int second(SA3DSOwnedTaskList*l,uint32_t id,void*v){(void)l;(void)id;event(v,'B');return 1;}
static int destructor(SA3DSOwnedTaskList*l,uint32_t id,void*v){
 Context*c=v;event(c,'D');assert(sa3ds_owned_task(l,id));
 if(c->destructor_insert){c->spawned=sa3ds_owned_task_allocate(l,constructor,NULL);assert(c->spawned!=id);c->destructor_insert=0;}
 return 1;
}
static int free_main(SA3DSOwnedTaskList*l,uint32_t id,void*v){event(v,'F');return sa3ds_owned_task_release(l,id,v);}
int main(void){
 SA3DSOwnedTask slots[5];SA3DSOwnedTaskList l;Context c={0};assert(sa3ds_owned_tasks_init(&l,slots,5));
 uint32_t b=sa3ds_owned_task_allocate(&l,second,NULL),a=sa3ds_owned_task_allocate(&l,first,NULL);assert(l.head==a&&slots[a-1].next==b&&slots[b-1].previous==a);
 c.insert=1;assert(sa3ds_owned_tasks_run(&l,&c)&&!strcmp(c.order,"AB")); /*New constructor waits.*/
 c.used=0;c.order[0]=0;assert(sa3ds_owned_tasks_run(&l,&c)&&!strcmp(c.order,"CAB"));
 c.used=0;c.order[0]=0;c.replace=1;c.target=b;assert(sa3ds_owned_tasks_run(&l,&c)&&!strcmp(c.order,"MAM")); /*Live later callback replacement.*/
 assert(sa3ds_owned_task_set_main(&l,a,free_main));sa3ds_owned_task(&l,a)->destructor=destructor;
 c.used=0;c.order[0]=0;c.destructor_insert=1;assert(sa3ds_owned_tasks_run(&l,&c)&&!strcmp(c.order,"MFDM"));
 assert(!sa3ds_owned_task(&l,a)&&l.head==c.spawned&&slots[b-1].previous!=a); /*Destructor's new head survives unlink.*/
 uint32_t reused=sa3ds_owned_task_allocate(&l,constructor,(void*)&c);assert(reused==a&&l.head==a&&!slots[a-1].destructor&&slots[a-1].data==&c);
 c.used=0;c.order[0]=0;assert(sa3ds_owned_tasks_run(&l,&c)&&!strcmp(c.order,"CCMM"));
 while(sa3ds_owned_task_allocate(&l,second,NULL)){}assert(!sa3ds_owned_task_allocate(&l,second,NULL));
 assert(!sa3ds_owned_task(&l,0)&&!sa3ds_owned_task(&l,6));
 assert(sa3ds_owned_task_set_main(&l,b,NULL)); /*Source dispatcher skips a null callback.*/
 puts("Owned task list: PASS head prepend,next-link snapshot,constructor-to-main timing,live future callback replacement,self-free continuation,destructor prepend/live unlink,LIFO identity reuse and bounded exhaustion; native integration/hierarchy separate");
}
