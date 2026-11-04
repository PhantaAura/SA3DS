#include "whale_motion_phase.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
/*Controlled service values test call/rounding boundaries; not a replacement
 *for the owner's Ninja lookup table in gameplay.*/
static double sine(void*unused,uint16_t angle){(void)unused;if(!angle)return 0;if(angle==0x4000)return 1;if(angle==0xc7f)return .25;return 0;}
static SA3DSWhaleMotionServices services={NULL,sine};
int main(void){
 float p[3]={0,0,0};SA3DSWhaleMotion a;assert(sa3ds_whale_motion_init(&a,p,0,&services));
 assert(a.action==1&&a.frame==8&&a.frame_step==.3f&&a.draw_y==-100&&a.counter==18);
 assert(a.roll==-2399&&a.yaw==0xc7f&&a.heading[0]==0&&a.heading[2]==1);
 assert(a.position[0]==-4.5f&&a.position[2]==0&&a.phase_time==-10&&a.curve_base==100);
 SA3DSWhaleMotionInput i={0};SA3DSWhaleMotionResult r;
 for(unsigned n=0;n<9;n++)assert(sa3ds_whale_motion_phase(&a,&i,&services,&r)&&a.action==1&&!r.clear_collision_disable);
 assert(sa3ds_whale_motion_phase(&a,&i,&services,&r)&&a.action==4&&a.draw_y==0&&r.clear_collision_disable);
 a=(SA3DSWhaleMotion){.action=3,.frame=7.7f,.frame_step=.3f,.phase_time=-1,.curve=.625f,.curve_base=-40,.yaw_step=80};
 assert(sa3ds_whale_motion_phase(&a,&i,&services,&r)&&a.frame==8&&a.action==3&&!r.surface_splash); /*unrounded frame compare*/
 assert(sa3ds_whale_motion_phase(&a,&i,&services,&r)&&a.action==4&&r.surface_splash&&r.rumble&&a.phase_time==-10);
 a=(SA3DSWhaleMotion){.action=4,.frame=17.8f,.frame_step=.2f,.phase_time=0,.curve=-1,.curve_base=100,.roll_step=72,.counter=11};
 assert(sa3ds_whale_motion_phase(&a,&i,&services,&r)&&a.frame==18&&a.action==5&&a.phase_time==0&&a.counter==0&&a.roll==72); /*stored frame compare*/
 a=(SA3DSWhaleMotion){.action=5,.frame=29.7f,.frame_step=.3f,.yaw_step=80,.roll_step=72,.counter=17};
 assert(sa3ds_whale_motion_phase(&a,&i,&services,&r)&&a.action==3&&a.frame==0&&a.frame_step==.2f);
 assert(a.yaw_step==-80&&a.roll_step==-72&&a.cycle==1&&a.counter==0&&a.phase_time==-8&&a.draw_y==0);
 assert(r.cycle_splash&&r.sound2f&&!r.allow_free_camera);
 for(unsigned request=0;request<3;request++){
  a=(SA3DSWhaleMotion){.action=5,.frame=29.7f,.frame_step=.3f};i.kill_request=request;
  assert(sa3ds_whale_motion_phase(&a,&i,&services,&r));
  assert(a.action==(request==1?2:3));
  if(request==1)assert(a.dive_velocity[0]==0&&a.dive_velocity[1]==-2.5f&&a.dive_velocity[2]==3);
 }
 i.kill_request=0;a=(SA3DSWhaleMotion){.action=2,.frame=29.6f,.draw_y=-347.5f,.dive_velocity={0,-2.5f,0}};
 assert(sa3ds_whale_motion_phase(&a,&i,&services,&r)&&a.frame>0&&a.frame<.000001f&&a.draw_y==-350);
 assert(r.destroy&&r.allow_free_camera&&!r.stop_common_tail); /*mark-delete still permits source common tail*/
 a=(SA3DSWhaleMotion){.action=1,.draw_y=-100,.dive_velocity={0,10,0}};i.camera_position[0]=1000;
 assert(sa3ds_whale_motion_phase(&a,&i,&services,&r)&&!r.destroy&&a.draw_y==-90); /*strict far bound*/
 i.camera_position[0]=1000.01f;SA3DSWhaleMotion before=a;
 assert(sa3ds_whale_motion_phase(&a,&i,&services,&r)&&r.destroy&&r.stop_common_tail&&!memcmp(&a,&before,sizeof(a)));
 puts("Whale motion phases: PASS source constructor/truncation,emergence,phase-specific float stores,cycle/kill timing,dive deletion/far boundary; common tail/native chase pending");
}
