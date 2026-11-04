#include "whale_actor.h"
#include "whale_draw.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {char order[64];unsigned used;float previous[3],center[3];SA3DSWhaleMotionResult phase;} Context;
static void event(Context*c,char e){assert(c->used+1<64);c->order[c->used++]=e;c->order[c->used]=0;}
static double sine(void*c,uint16_t a){(void)c;if(!(a&0x3fff)){const double values[]={0,1,0,-1};return values[a>>14];}return sin(a*6.2831853071795864769/65536.0);}
static int history(void*v,float p[3]){event(v,'H');memset(p,0,12);return 1;}
static int32_t character(void*v){event(v,'C');return -1;}
static void bounce(void*v,const float p[3]){(void)p;event(v,'B');}
static void enqueue(void*v,const float p[3]){Context*c=v;event(c,'E');memcpy(c->center,p,12);}
static uint16_t random15(void*v){event(v,'R');return 0;}
static void wave(void*v,float x,float z,float zero,float scale,float size){(void)x;(void)z;(void)zero;(void)scale;(void)size;event(v,'W');}
static void display(void*v){event(v,'D');}
static void phase(void*v,const SA3DSWhaleMotionResult*r,const float old[3]){Context*c=v;event(c,'P');c->phase=*r;memcpy(c->previous,old,12);}
static void wi(unsigned char*p,int32_t i){uint32_t u;memcpy(&u,&i,4);for(unsigned k=0;k<4;k++)p[k]=(unsigned char)(u>>(k*8));}
int main(void){
 Context c={0};SA3DSWhaleActorServices s={{NULL,sine},{&c,sine,history,character,bounce,enqueue,random15,wave,display},phase,&c};
 unsigned char keys[128]={0};SA3DSWhaleTipAssets tip={keys};SA3DSWhaleActor a;SA3DSWhaleActorResult r;
 float p[3]={0,0,0},old[3]={5,6,7};assert(sa3ds_whale_actor_init(&a,p,0,old,&s.motion)&&a.collision_disabled&&!memcmp(a.shared_tip,old,12));
 SA3DSWhaleMotionInput input={0};
 for(unsigned tick=0;tick<10;tick++){
  memset(&c,0,sizeof(c));memcpy(input.camera_position,a.motion.position,12);
  assert(sa3ds_whale_actor_tick(&a,&tip,&input,0,&s,&r));
  if(tick<9)assert(a.motion.action==1&&a.collision_disabled&&c.center[1]==-100000);
  else assert(a.motion.action==4&&!a.collision_disabled&&c.phase.clear_collision_disable&&c.previous[1]==-100000&&c.center[1]!=-100000&&c.order[0]=='P');
  if(!tick)assert(!strcmp(c.order,"HERRWD")&&a.motion.counter==0);
 }
 memset(&c,0,sizeof(c));a.motion.action=3;a.motion.frame=7.9f;a.motion.frame_step=.2f;memcpy(a.shared_tip,old,12);memcpy(input.camera_position,a.motion.position,12);
 assert(sa3ds_whale_actor_tick(&a,&tip,&input,0,&s,&r)&&c.phase.surface_splash&&c.phase.rumble&&!memcmp(c.previous,old,12)&&c.order[0]=='P');
 memset(&c,0,sizeof(c));a.motion.action=2;a.motion.draw_y=-349;a.motion.dive_velocity[1]=-2.5f;a.motion.counter=0;memcpy(input.camera_position,a.motion.position,12);
 assert(sa3ds_whale_actor_tick(&a,&tip,&input,0,&s,&r)&&r.phase.destroy&&!r.phase.stop_common_tail&&!strcmp(c.order,"PHED"));
 memset(&c,0,sizeof(c));input.camera_position[0]+=2000;
 assert(sa3ds_whale_actor_tick(&a,&tip,&input,0,&s,&r)&&r.phase.destroy&&r.phase.stop_common_tail&&!strcmp(c.order,"P"));
 wi(keys,20);wi(keys+29*4,100);wi(keys+30*4,200);wi(keys+31*4,300);int32_t angle;
 assert(sa3ds_whale_cycle_pitch(&tip,29.5f,&angle)&&angle==150);
 assert(sa3ds_whale_cycle_pitch(&tip,31.5f,&angle)&&angle==160);
 assert(sa3ds_whale_cycle_pitch(&tip,32,&angle)&&angle==20);
 wi(keys,-5);wi(keys+4,2);assert(sa3ds_whale_cycle_pitch(&tip,.5f,&angle)&&angle==-1);
 assert(!sa3ds_whale_cycle_pitch(&tip,-1,&angle));
 SA3DSWhaleMotion m={0};m.position[0]=10;m.position[1]=20;m.position[2]=30;m.draw_y=7;m.yaw=m.roll=0x4000;SA3CMatrix matrix;unsigned bank;
 assert(sa3ds_whale_draw_transform(&m,0x4000,&s.motion,&matrix,&bank)&&bank==0);
 assert(matrix.m[0]==0&&matrix.m[1]==1&&matrix.m[2]==0&&matrix.m[4]==1&&matrix.m[5]==0&&matrix.m[6]==0&&matrix.m[8]==0&&matrix.m[9]==0&&matrix.m[10]==-1);
 assert(matrix.m[12]==10&&matrix.m[13]==27&&matrix.m[14]==30);
 m.action=2;assert(sa3ds_whale_draw_transform(&m,0,&s.motion,&matrix,&bank)&&bank==1);
 puts("Whale actor/draw: PASS emergence/CCL enable,previous-tip phase effects,pre-movement common order,dive/far deletion distinction,cycle root wrap/truncation,Y/Z/X transform order and bank selection; native services/exit root separate");
}
