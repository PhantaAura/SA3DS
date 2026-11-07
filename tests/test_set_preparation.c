#include "set_preparation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {uint32_t tasks[4];unsigned count;int fail;} Context;
static int request(void*v,uint32_t task){Context*c=v;if(c->fail)return 0;c->tasks[c->count++]=task;return 1;}
int main(void){
 SA3DSSetTaskLifetime e[4];memset(e,0xa5,sizeof(e));
 const uint16_t types[4]={67,68,0x1043,29};
 /*Cold preparation zeroes stale work and marks every row eligible.*/
 assert(sa3ds_set_cold_prepare(e,types,4,1,0,0));
 for(unsigned i=0;i<4;i++)assert(e[i].flags==0x8000&&!e[i].loads&&!e[i].task&&!e[i].free_main&&sa3ds_set_task_load_eligible(&e[i]));
 /*EC level1 act0 type67 composite is0x143; an unimported mission-table match rejects.*/
 assert(!sa3ds_set_cold_prepare(e,types,4,1,0,0x143));
 assert(sa3ds_set_cold_prepare(e,types,4,1,1,0x143));
 assert(!sa3ds_set_cold_prepare(e,types,0x400,1,0,0));
 /*Loaded entry: act clear keeps bit0/association/0x8 and requests40B570.*/
 assert(sa3ds_set_task_load_commit(&e[0],7,160000));
 e[0].flags|=0x8|0x10|0x4|0x2;e[1].flags|=0x4000|0x10;e[2].flags|=0x8;
 Context c={0};assert(sa3ds_set_act_clear(e,4,request,&c));
 assert(c.count==1&&c.tasks[0]==7&&e[0].flags==(0x1|0x8)&&e[0].task==7&&e[0].free_main);
 assert(e[1].flags==(0x8000|0x4000|0x10)&&e[2].flags==0x8&&e[3].flags==0);
 /*Deferred free runs destructor and clears only bit0/association.*/
 uint8_t freed;assert(sa3ds_set_task_run_free(&e[0],7,1,NULL,NULL,&freed)&&freed&&e[0].flags==0x8&&!e[0].task&&e[0].loads==1);
 /*Compaction drops unpreserved rows and clears0x4000 on preserved rows.*/
 assert(sa3ds_set_act_compact(e,4));
 assert(!e[0].flags&&!e[0].loads&&e[1].flags==(0x8000|0x10)&&!e[2].flags&&!e[3].flags);
 /*Null association with active bit faults in source; port reports error.*/
 SA3DSSetTaskLifetime bad={.flags=0x8001};assert(!sa3ds_set_act_clear(&bad,1,request,&c));
 c.fail=1;bad.task=3;assert(!sa3ds_set_act_clear(&bad,1,request,&c));
 SA3DSSetTaskLifetime restored={.flags=2};assert(!sa3ds_set_act_compact(&restored,1));
 /*Restart reset: activated/persistent0x10 rows survive, others reload cleanly.*/
 SA3DSSetTaskLifetime r[2]={{.flags=0x8014,.loads=3},{.flags=0x000c,.loads=9}};
 assert(sa3ds_set_restart_reset(r,2)&&r[0].flags==0x8014&&r[0].loads==3&&r[1].flags==0x8000&&!r[1].loads);
 puts("SET preparation: PASS cold mark/zero,mission-match rejection,act clear 7FE9/deferred 40B570,compaction,restart 0x10 preservation");
}
