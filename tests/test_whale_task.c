#include "set_preparation.h"
#include "whale_task.h"
#include "whale_trigger_runtime.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
 SA3DSWhaleTriggerFrame frame;SA3DSWhaleTaskRuntime *whales;float camera[3],history[3];
 unsigned camera_disables,rumble_stops,bounces,waves,effects,destroy_requests;uint32_t player_id;
} Context;
static double sine(void*c,uint16_t a){(void)c;if(!(a&0x3fff)){const double v[]={0,1,0,-1};return v[a>>14];}return sin(a*6.2831853071795864769/65536.0);}
static int read_frame(void*v,SA3DSWhaleTriggerFrame*f){*f=((Context*)v)->frame;return 1;}
static int summon(void*v,const float p[3],int32_t yaw){return sa3ds_whale_tasks_summon(((Context*)v)->whales,p,yaw);}
static void kill(void*v){(void)sa3ds_whale_tasks_kill(((Context*)v)->whales);}
static int camera(void*v,float p[3]){memcpy(p,((Context*)v)->camera,12);return 1;}
static int history(void*v,float p[3]){memcpy(p,((Context*)v)->history,12);return 1;}
static int32_t character(void*v,uint32_t other){return other&&other==((Context*)v)->player_id?0:-1;}
static void bounce(void*v,const float p[3]){(void)p;((Context*)v)->bounces++;}
static uint16_t random15(void*v){(void)v;return 0;}
static void wave(void*v,float x,float z,float zero,float scale,float size){(void)x;(void)z;(void)zero;(void)scale;(void)size;((Context*)v)->waves++;}
static void effects(void*v,const SA3DSWhaleMotionResult*r,const float old[3]){(void)old;Context*c=v;c->effects++;if(r->destroy)c->destroy_requests++;}
static void camera_disable(void*v){((Context*)v)->camera_disables++;}
static void rumble_stop(void*v){((Context*)v)->rumble_stops++;}
static void w16(unsigned char*p,uint16_t u){p[0]=(unsigned char)u;p[1]=(unsigned char)(u>>8);}
static void w32(unsigned char*p,uint32_t u){for(unsigned k=0;k<4;k++)p[k]=(unsigned char)(u>>(8*k));}
static void wf(unsigned char*p,float f){uint32_t u;memcpy(&u,&f,4);w32(p,u);}
static SA3DSWhaleTaskSlot*live(SA3DSWhaleTaskRuntime*r){
 for(unsigned i=0;i<SA3DS_WHALE_TASK_SLOTS;i++){SA3DSWhaleTaskSlot*s=sa3ds_whale_tasks_slot(r,i);if(s&&s->task==r->shared.current_task)return s;}
 return NULL;
}
int main(void){
 unsigned char rows[80]={0};w32(rows+4,5);w16(rows+8,67);w16(rows+12,0x4000);
 wf(rows+16,10);wf(rows+20,20);wf(rows+24,30);wf(rows+28,1);wf(rows+32,2);wf(rows+36,3);
 w32(rows+44,8);w16(rows+48,68);wf(rows+56,200);
 SA3DSWhaleTriggerAssets a={rows,2,45,160000};
 SA3DSOwnedTask slots[8];SA3DSOwnedTaskList list;assert(sa3ds_owned_tasks_init(&list,slots,8));
 unsigned char pitch[128]={0};SA3DSWhaleTipAssets tip={.pitch=pitch,.exit_root_bind=1};
 SA3DSWhaleCCLAssets ccl={0};ccl.broad_radius=150;ccl.rules=(SA3DSCCLRules){2,32,64,0x80000};
 ccl.shapes[0]=(SA3DSCCLShape){0,0x70,0x21,0x420,{0,0,0},40};ccl.shapes[1]=(SA3DSCCLShape){0,0x70,0,0,{0,0,0},60};
 SA3DSWhaleTaskRuntime whales;Context c={.whales=&whales,.player_id=1};
 SA3DSWhaleTaskServices ws={&c,sine,camera,history,character,bounce,random15,wave,effects,camera_disable,rumble_stop};
 assert(sa3ds_whale_tasks_init(&whales,&list,&tip,&ccl,&ws));whales.ccl_id_base=0x30000;
 /*46C1D0/46BD20 cold preparation supplies the trigger rows' SET flags.*/
 SA3DSSetTaskLifetime prepared[2];const uint16_t types[2]={67,68};
 assert(sa3ds_set_cold_prepare(prepared,types,2,1,0,0));
 const uint16_t flags[2]={prepared[0].flags,prepared[1].flags};
 SA3DSWhaleTriggerRuntime triggers;SA3DSWhaleTriggerEntry entries[2];
 c.frame.clip.player0.present=c.frame.clip.camera.present=1;c.frame.clip_level=1;
 c.frame.players[0]=(SA3DSWhaleContactPlayer){{10,20,30},1,1};
 SA3DSWhaleTriggerServices ts={&c,read_frame,summon,kill,NULL};
 assert(sa3ds_whale_triggers_init(&triggers,&a,&list,entries,flags,2,&ts)&&sa3ds_whale_triggers_load(&triggers));
 assert(sa3ds_owned_tasks_run(&list,&c)&&sa3ds_owned_tasks_run(&list,&c)&&!whales.spawns); /*construct, arm*/
 assert(sa3ds_owned_tasks_run(&list,&c)&&whales.spawns==1&&whales.shared.current_task&&!whales.constructs);
 SA3DSWhaleTaskSlot*s=&whales.slots[0];assert(s->used&&!s->constructed&&s->spawn_position[0]==11&&s->spawn_yaw==0x4000);
 /*The actor constructor waits for the next list traversal (40B0C0 head insertion).*/
 memcpy(c.camera,s->spawn_position,12);memcpy(c.history,s->spawn_position,12);
 c.frame.players[0].center[0]=-500; /*leave both triggers*/
 assert(sa3ds_owned_tasks_run(&list,&c)&&whales.constructs==1&&c.camera_disables==1&&s->constructed&&!s->displayed);
 assert(s->ccl.shapes[1].attributes&0x10&&s->ccl.body.hits.actor_id==whales.ccl_id_base+s->task);
 assert(sa3ds_owned_tasks_run(&list,&c)&&s->displayed&&s->ccl_queued&&s->ccl.shapes[0].center.y==-100000&&c.waves==1);
 assert(s->bank==0&&s->draw.m[15]==1);
 for(unsigned i=0;i<9;i++){memcpy(c.camera,s->actor.motion.position,12);assert(sa3ds_owned_tasks_run(&list,&c));}
 assert(!(s->ccl.shapes[1].attributes&0x10)&&!s->actor.collision_disabled&&s->ccl.shapes[0].center.y!=-100000);
 assert(!memcmp(whales.shared.tip,s->actor.shared_tip,12));
 /*Prior CCL bit8 with Sonic cached: 441AD0 returns character0 -> bounce.*/
 s->ccl.body.status|=8;s->ccl.body.cached_other_id=1;
 memcpy(c.camera,s->actor.motion.position,12);assert(sa3ds_owned_tasks_run(&list,&c)&&c.bounces==1);
 s->ccl.body.status&=(uint16_t)~8u;
 /*Kill trigger: repeated4F8910 requests until the AO task deletes itself.*/
 c.frame.players[0].center[0]=200;c.frame.players[0].center[1]=c.frame.players[0].center[2]=0;
 uint32_t old=whales.shared.current_task;unsigned ticks=0;
 while(whales.destructs==0&&ticks<4000){memcpy(c.camera,s->actor.motion.position,12);memcpy(c.history,s->actor.motion.position,12);assert(sa3ds_owned_tasks_run(&list,&c));ticks++;}
 assert(whales.destructs==1&&whales.deletion_requests==1&&c.destroy_requests>=1&&c.rumble_stops==1);
 assert(!whales.shared.current_task&&!whales.shared.tip[0]&&!whales.shared.tip[1]&&!whales.shared.tip[2]&&!s->used);
 assert(whales.shared.kill_request==1); /*4F8930 leaves the shared kill word.*/
 /*Replacement: old task deferred, new current; old destructor cannot clobber.*/
 float p[3]={1,2,3};assert(sa3ds_whale_tasks_summon(&whales,p,0)&&!whales.shared.kill_request);
 uint32_t first=whales.shared.current_task;assert(first==old); /*LIFO slot identity reuse.*/
 assert(sa3ds_owned_tasks_run(&list,&c));memcpy(c.camera,p,12);
 float tip_before[3]={4,5,6};memcpy(whales.shared.tip,tip_before,12);
 assert(sa3ds_whale_tasks_summon(&whales,p,0));uint32_t second=whales.shared.current_task;assert(second!=first);
 unsigned before=whales.destructs;assert(sa3ds_owned_tasks_run(&list,&c));
 assert(whales.destructs==before+1&&whales.shared.current_task==second&&!memcmp(whales.shared.tip,tip_before,12)&&c.rumble_stops==2);
 assert(live(&whales)&&live(&whales)->task==second);
 /*A replaced task whose constructor never ran has no destructor (5022A0).*/
 assert(sa3ds_whale_tasks_summon(&whales,p,0));uint32_t third=whales.shared.current_task;
 assert(sa3ds_whale_tasks_summon(&whales,p,0));before=whales.destructs;unsigned stops=c.rumble_stops;
 assert(sa3ds_owned_tasks_run(&list,&c)&&whales.destructs==before+1&&c.rumble_stops==stops+1&&third!=whales.shared.current_task);
 /*Bounded slots: exhaustion is reported and leaves a null current task.*/
 SA3DSOwnedTask few[8];SA3DSOwnedTaskList fl;SA3DSWhaleTaskRuntime fw;assert(sa3ds_owned_tasks_init(&fl,few,8)&&sa3ds_whale_tasks_init(&fw,&fl,&tip,&ccl,&ws));
 for(unsigned i=0;i<SA3DS_WHALE_TASK_SLOTS;i++)assert(sa3ds_whale_tasks_summon(&fw,p,0));
 assert(!sa3ds_whale_tasks_summon(&fw,p,0)&&fw.allocation_failures==1&&!fw.shared.current_task);
 assert(!sa3ds_whale_tasks_kill(&fw));
 printf("Whale task executor: PASS cold SET flags,trigger summon,deferred constructor,camera disable,CCL second-shape emergence,"
  "tip publication,bit8 bounce,kill-to-dive deletion(%u ticks),guarded destructor,replacement,pre-constructor free,slot exhaustion\n",ticks);
}
