#include "whale_motion_phase.h"
#include <math.h>
#include <string.h>
static int32_t add_angle(int32_t a,int32_t b){uint32_t bits=(uint32_t)a+(uint32_t)b;int32_t out;memcpy(&out,&bits,4);return out;}
static int32_t negate_angle(int32_t a){uint32_t bits=0u-(uint32_t)a;int32_t out;memcpy(&out,&bits,4);return out;}
static double sine(const SA3DSWhaleMotionServices*s,int32_t angle){return s->sine(s->context,(uint16_t)angle);}
int sa3ds_whale_motion_init(SA3DSWhaleMotion*m,const float p[3],int32_t yaw,const SA3DSWhaleMotionServices*s){
 if(!m||!p||!s||!s->sine)return 0;
 SA3DSWhaleMotion a={0};
 for(unsigned k=0;k<3;k++){if(!isfinite(p[k]))return 0;a.position[k]=p[k];}
 double sy=sine(s,yaw),cy=sine(s,add_angle(yaw,0x4000)),bend=sine(s,0xc7f);
 if(!isfinite(sy)||!isfinite(cy)||!isfinite(bend))return 0;
 a.action=1;a.frame=8;a.frame_step=.3f;a.draw_y=-100;
 a.phase_time=-10;a.curve=-1;a.curve_base=100;
 a.heading[0]=(float)sy;a.heading[2]=(float)cy;a.dive_velocity[1]=10;
 a.base_yaw=yaw;a.yaw=add_angle(yaw,0xc7f);a.yaw_step=80;a.roll_step=72;
 /*Original FILD72 * float33.333332 then truncation, not rounded -2400.*/
 a.roll=(int32_t)(-(double)a.roll_step*33.333332061767578125f);
 a.counter=18;
 double bend_scale=bend*(double)1.8f;
 a.position[0]=(float)((double)a.position[0]-bend_scale*a.heading[2]*10.0);
 a.position[2]=(float)((double)a.position[2]+bend_scale*a.heading[0]*10.0);
 *m=a;return 1;
}
int sa3ds_whale_motion_phase(SA3DSWhaleMotion*m,const SA3DSWhaleMotionInput*i,
 const SA3DSWhaleMotionServices*s,SA3DSWhaleMotionResult*out){
 if(!m||!i||!s||!s->sine||!out)return 0;
 memset(out,0,sizeof(*out));
 double delta[3];for(unsigned k=0;k<3;k++){
  if(!isfinite(i->player_position[k])||!isfinite(m->position[k]))return 0;
  delta[k]=(double)i->player_position[k]-m->position[k];
 }
 if(delta[2]*delta[2]+delta[1]*delta[1]+delta[0]*delta[0]>1000000.0){out->destroy=out->stop_common_tail=1;return 1;}
 switch(m->action){
 case 1:{
  double rise=(double)m->draw_y+m->dive_velocity[1];m->draw_y=(float)rise;
  if(rise>=0){m->draw_y=0;m->action=4;out->clear_collision_disable=1;}
  break;
 }
 case 2:{
  m->position[0]=(float)((double)m->position[0]+m->dive_velocity[0]);
  m->position[2]=(float)((double)m->position[2]+m->dive_velocity[2]);
  m->draw_y=(float)((double)m->draw_y+m->dive_velocity[1]);
  double frame=(double)m->frame+.4f;m->frame=(float)(frame>30?frame-30:frame);
  if(m->draw_y<=-350)out->destroy=1;
  out->allow_free_camera=1;break;
 }
 case 3:{
  double frame=(double)m->frame+m->frame_step;m->frame=(float)frame;
  float time=(float)((double)m->phase_time+m->frame_step);
  m->yaw=add_angle(m->yaw,m->yaw_step);
  m->draw_y=(float)((double)time*time*m->curve+m->curve_base);
  if(frame>=8){
   m->action=4;m->curve=-1;m->curve_base=(float)((double)m->draw_y+100);
   time=-10;out->surface_splash=out->rumble=1;
  }
  m->phase_time=time;break;
 }
 case 4:{
  m->frame=(float)((double)m->frame+m->frame_step);
  double time=(double)m->phase_time+m->frame_step;
  m->roll=add_angle(m->roll,m->roll_step);
  m->draw_y=(float)(time*time*m->curve+m->curve_base);
  if(m->frame>=18){
   m->frame_step=.3f;m->action=5;
   m->curve=(float)((double)m->draw_y*-.0069444444961845875f);
   m->curve_base=m->draw_y;m->counter=0;time=0;
  }
  m->phase_time=(float)time;break;
 }
 case 5:{
  m->frame=(float)((double)m->frame+m->frame_step);
  float time=(float)((double)m->phase_time+m->frame_step);
  m->yaw=add_angle(m->yaw,negate_angle(m->yaw_step));m->roll=add_angle(m->roll,m->roll_step);
  m->draw_y=(float)((double)time*time*m->curve+m->curve_base);
  if(m->frame>=30){
   m->frame_step=.2f;m->frame=(float)((double)m->frame-30);m->action=3;m->draw_y=0;
   m->yaw_step=negate_angle(m->yaw_step);m->roll_step=negate_angle(m->roll_step);
   m->curve=.625f;m->curve_base=-40;m->cycle++;m->counter=0;time=-8;
   out->cycle_splash=out->sound2f=1;
   if(i->kill_request==1){
    double sy=sine(s,m->yaw),cy=sine(s,add_angle(m->yaw,0x4000));
    if(!isfinite(sy)||!isfinite(cy))return 0;
    m->action=2;m->dive_velocity[0]=(float)((double)(float)sy*3);
    m->dive_velocity[1]=-2.5f;m->dive_velocity[2]=(float)(cy*3);
   }
  }
  m->phase_time=time;break;
 }
 }
 return 1;
}
