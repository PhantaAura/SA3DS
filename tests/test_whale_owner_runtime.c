/*Owner-package integration: real AO trigger rows, v2 tip and CCL profile drive
 *the native executor. libm sine stands in for the owner Ninja table; this is
 *a package/flow gate, not a retail trajectory comparison.*/
#include "set_preparation.h"
#include "whale_task.h"
#include "whale_trigger_runtime.h"
#include "whale_visual.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {SA3DSWhaleTriggerFrame frame;SA3DSWhaleTaskRuntime *whales;float player[3];unsigned splash,rumble,waves;} Context;
static double sine(void*c,uint16_t a){(void)c;if(!(a&0x3fff)){const double v[]={0,1,0,-1};return v[a>>14];}return sin(a*6.2831853071795864769/65536.0);}
static int read_frame(void*v,SA3DSWhaleTriggerFrame*f){*f=((Context*)v)->frame;return 1;}
static int summon(void*v,const float p[3],int32_t yaw){return sa3ds_whale_tasks_summon(((Context*)v)->whales,p,yaw);}
static void kill(void*v){(void)sa3ds_whale_tasks_kill(((Context*)v)->whales);}
static int camera(void*v,float p[3]){Context*c=v;p[0]=c->player[0];p[1]=c->player[1]+18;p[2]=c->player[2]-40;return 1;}
static int history(void*v,float p[3]){memcpy(p,((Context*)v)->player,12);return 1;}
static int32_t character(void*v,uint32_t o){(void)v;return o==1?0:-1;}
static void bounce(void*v,const float p[3]){(void)v;(void)p;}
static uint16_t random15(void*v){(void)v;static uint32_t r=1;r=r*214013u+2531011u;return (uint16_t)((r>>16)&32767u);}
static void wave(void*v,float x,float z,float a,float b,float s){(void)x;(void)z;(void)a;(void)b;(void)s;((Context*)v)->waves++;}
static void effects(void*v,const SA3DSWhaleMotionResult*r,const float o[3]){(void)o;Context*c=v;c->splash+=r->surface_splash+r->cycle_splash;c->rumble+=r->rumble;}
static unsigned char*load(const char*dir,const char*name,size_t*size){
 char path[1024];snprintf(path,sizeof(path),"%s/%s",dir,name);FILE*f=fopen(path,"rb");assert(f);
 fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);unsigned char*b=malloc((size_t)n);assert(b&&fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);*size=(size_t)n;return b;
}
int main(int argc,char**argv){
 assert(argc==2);size_t ts,ps,cs,vs;
 unsigned char*tb=load(argv[1],"whale_triggers.sa3dspak",&ts),*pb=load(argv[1],"whale_tip.sa3dspak",&ps);
 unsigned char*cb=load(argv[1],"whale_ccl.sa3dspak",&cs),*vb=load(argv[1],"whale_visual.sa3dspak",&vs);
 SA3DSWhaleTriggerAssets ta;SA3DSWhaleTipAssets tip;SA3DSWhaleCCLAssets ccl;SA3DSWhaleVisual visual;
 assert(sa3ds_whale_trigger_assets_open(&ta,tb,ts)&&ta.count==4);
 assert(sa3ds_whale_tip_assets_open(&tip,pb,ps)&&tip.exit_root_bind);
 assert(sa3ds_whale_ccl_assets_open(&ccl,cb,cs)&&ccl.broad_radius==150&&ccl.shapes[0].radius==40&&ccl.shapes[1].radius==60);
 assert(sa3ds_whale_visual_open(&visual,vb,vs));
 SA3DSOwnedTask slots[16];SA3DSOwnedTaskList list;assert(sa3ds_owned_tasks_init(&list,slots,16));
 SA3DSWhaleTaskRuntime whales;Context c={.whales=&whales};
 SA3DSWhaleTaskServices ws={&c,sine,camera,history,character,bounce,random15,wave,effects,NULL,NULL};
 assert(sa3ds_whale_tasks_init(&whales,&list,&tip,&ccl,&ws));whales.ccl_id_base=0x30000;
 SA3DSSetTaskLifetime set[4];uint16_t types[4],flags[4];SA3DSWhalePlacement rows[4];int summon_row=-1,kill_row=-1;
 for(uint32_t i=0;i<4;i++){
  assert(sa3ds_whale_trigger_placement(&ta,i,&rows[i])&&rows[i].act==0);types[i]=rows[i].type;
  if((rows[i].type&4095)==67&&summon_row<0)summon_row=(int)i;
  if((rows[i].type&4095)==68&&kill_row<0)kill_row=(int)i;
 }
 assert(summon_row>=0&&kill_row>=0&&sa3ds_set_cold_prepare(set,types,4,1,0,0));
 for(unsigned i=0;i<4;i++)flags[i]=set[i].flags;
 SA3DSWhaleTriggerRuntime tr;SA3DSWhaleTriggerEntry entries[4];SA3DSWhaleTriggerServices tsv={&c,read_frame,summon,kill,NULL};
 assert(sa3ds_whale_triggers_init(&tr,&ta,&list,entries,flags,4,&tsv));
 const SA3DSWhalePlacement*s=&rows[summon_row];memcpy(c.player,s->position,12);
 c.frame.clip_level=1;c.frame.clip.camera.present=c.frame.clip.player0.present=1;
 unsigned tick=0;uint32_t emerged_tick=0,max_draw_y_ticks=0;
 for(;tick<1200;tick++){
  memcpy(c.frame.clip.player0.position,c.player,12);camera(&c,c.frame.clip.camera.position);
  c.frame.players[0]=(SA3DSWhaleContactPlayer){{c.player[0],c.player[1]+5,c.player[2]},1,1};
  assert(sa3ds_whale_triggers_load(&tr)&&sa3ds_owned_tasks_run(&list,&c));
  for(unsigned i=0;i<SA3DS_WHALE_TASK_SLOTS;i++){
   SA3DSWhaleTaskSlot*w=sa3ds_whale_tasks_slot(&whales,i);if(!w||!w->displayed)continue;
   assert(w->bank<2&&isfinite(w->frame)&&isfinite(w->draw.m[12])&&isfinite(w->draw.m[13])&&isfinite(w->draw.m[14]));
   static float pos[512][3],nrm[512][3];static SA3DSECVertex out[2048];
   assert(visual.shape.vertices<=512&&visual.visual.vertex_count<=2048);
   assert(sa3ds_whale_visual_sample(&visual,w->bank,w->frame,pos,nrm,512,out,2048));
   if(!emerged_tick&&!(w->ccl.shapes[1].attributes&0x10))emerged_tick=tick;
   if(w->actor.motion.action==4)max_draw_y_ticks++;
  }
  if(emerged_tick&&tick>emerged_tick+120)break;
 }
 assert(whales.spawns==1&&whales.constructs==1&&emerged_tick&&max_draw_y_ticks);
 /*Follow the whale's kill trigger: repeated requests until its own deletion.*/
 memcpy(c.player,rows[kill_row].position,12);unsigned kill_ticks=0;
 while(!whales.destructs&&kill_ticks<4000){
  memcpy(c.frame.clip.player0.position,c.player,12);camera(&c,c.frame.clip.camera.position);
  c.frame.players[0]=(SA3DSWhaleContactPlayer){{c.player[0],c.player[1]+5,c.player[2]},1,1};
  assert(sa3ds_whale_triggers_load(&tr)&&sa3ds_owned_tasks_run(&list,&c));kill_ticks++;
 }
 assert(whales.destructs==1&&!whales.shared.current_task);
 printf("Whale owner runtime: PASS 4 owner AO rows (summon row %d, kill row %d), v2 tip, CCL 40/60/150, visual sampling;"
  " summon->construct->emerge at tick %u, chase %u ticks, kill->delete %u ticks; splash=%u rumble=%u waves=%u; libm sine, not retail trace\n",
  summon_row,kill_row,emerged_tick,max_draw_y_ticks,kill_ticks,c.splash,c.rumble,c.waves);
 free(tb);free(pb);free(cb);free(vb);
}
