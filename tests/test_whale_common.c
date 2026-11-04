#include "whale_common.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {float history[3],center[3],velocity[3],wave_x,wave_z;int character,history_calls,random_calls;char order[32];unsigned used;} Context;
static void event(Context*c,char e){assert(c->used+1<sizeof(c->order));c->order[c->used++]=e;c->order[c->used]=0;}
static double sine(void*v,uint16_t a){(void)v;assert(a==0||a==0x4000);return a?1:0;}
static int history(void*v,float p[3]){Context*c=v;c->history_calls++;event(c,'H');memcpy(p,c->history,12);return 1;}
static int32_t character(void*v){Context*c=v;event(c,'C');return c->character;}
static void bounce(void*v,const float p[3]){Context*c=v;event(c,'B');memcpy(c->velocity,p,12);}
static void collision(void*v,const float p[3]){Context*c=v;event(c,'E');memcpy(c->center,p,12);}
static uint16_t random15(void*v){Context*c=v;event(c,'R');return c->random_calls++?32767:0;}
static void wave(void*v,float x,float z,float zero,float scale,float size){Context*c=v;event(c,'W');c->wave_x=x;c->wave_z=z;assert(zero==0&&scale==1&&size==13);}
static void display(void*v){event(v,'D');}
static SA3DSWhaleMotion motion(float speed){SA3DSWhaleMotion m={0};m.action=4;m.heading[2]=1;m.tracking_speed=speed;return m;}
int main(void){
 Context c={0};SA3DSWhaleCommonServices s={&c,sine,history,character,bounce,collision,random15,wave,display};
 SA3DSWhaleCommonInput i={{10,20,30},0,0};SA3DSWhaleCommonResult r;
 const struct {float distance,speed,expected;} cases[]={
  {401,11.95f,12.05f},{401,12,12},{400,6,5.9f},{400,5.95f,6.05f},
  {200,1.05f,.95f},{200,1,1},{201,1,1.1f},{0,.95f,1}};
 for(unsigned n=0;n<sizeof(cases)/sizeof(cases[0]);n++){
  memset(&c,0,sizeof(c));c.history[2]=cases[n].distance;SA3DSWhaleMotion m=motion(cases[n].speed);
  assert(sa3ds_whale_common(&m,&i,&s,&r));assert(fabsf(m.tracking_speed-cases[n].expected)<.00001f);
  assert(m.position[2]==m.tracking_speed&&m.position[0]==0&&m.position[1]==0);
  assert(!strcmp(c.order,"HED")&&c.center[2]==30); /*Pre-movement tip.*/
 }
 memset(&c,0,sizeof(c));SA3DSWhaleMotion m=motion(9);m.yaw=0x4000;m.counter=18;
 i.kill_request=1;i.collision_flag8=1;
 assert(sa3ds_whale_common(&m,&i,&s,&r));
 assert(!c.history_calls&&m.tracking_speed==2&&r.bounced&&r.wave);
 assert(fabsf(m.position[0]-1.8f)<1e-6f&&m.position[2]==2);
 assert(c.velocity[0]==0&&c.velocity[1]==2&&c.velocity[2]==8);
 assert(!strcmp(c.order,"CBERRWD")&&c.random_calls==2&&m.counter==0);
 assert(c.wave_x==-115&&c.wave_z==154.99237060546875f);
 /*Kill value2 does not take the ==1 branch. Character1 never bounces player1.*/
 memset(&c,0,sizeof(c));c.character=1;i.kill_request=2;m=motion(2);
 assert(sa3ds_whale_common(&m,&i,&s,&r)&&c.history_calls==1&&!r.bounced&&!strcmp(c.order,"HCED"));
 /*Constructor's18 emits on first update; emergence hides CCL below the world.*/
 memset(&c,0,sizeof(c));i.collision_flag8=0;m=motion(1);m.action=1;m.counter=18;
 assert(sa3ds_whale_common(&m,&i,&s,&r)&&r.wave&&c.center[0]==0&&c.center[1]==-100000&&c.center[2]==0);
 assert(c.wave_x==-125&&c.wave_z==124.99237060546875f);
 /*Dive still enqueues/displays, without consuming RNG or resetting its clock.*/
 memset(&c,0,sizeof(c));m=motion(1);m.action=2;m.counter=18;
 assert(sa3ds_whale_common(&m,&i,&s,&r)&&!r.wave&&m.counter==19&&!strcmp(c.order,"HED"));
 m.counter=65535;assert(sa3ds_whale_common(&m,&i,&s,&r)&&m.counter==0);
 /*Source zero-speed branch avoids division but retains lateral displacement.*/
 memset(&c,0,sizeof(c));c.history[2]=401;m=motion(-.1f);m.yaw=0x4000;
 assert(sa3ds_whale_common(&m,&i,&s,&r)&&m.tracking_speed==0&&fabsf(m.position[0]-1.8f)<1e-6f&&m.position[2]==0);
 puts("Whale common: PASS speed boundaries/overshoot,kill equality,pre-movement CCL,character bounce,service order,RNG/waves,dive clock wrap and zero-speed wobble; native wiring separate");
}
