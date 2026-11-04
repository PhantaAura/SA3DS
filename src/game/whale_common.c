#include "whale_common.h"
#include <math.h>
#include <string.h>
int sa3ds_whale_common(SA3DSWhaleMotion*m,const SA3DSWhaleCommonInput*i,
 const SA3DSWhaleCommonServices*s,SA3DSWhaleCommonResult*out){
 if(!m||!i||!s||!out||!s->sine||!s->buffered_position||!s->collision_character||
    !s->bounce_player0||!s->enqueue_collision||!s->random15||!s->wave||!s->display)return 0;
 for(unsigned k=0;k<3;k++)if(!isfinite(i->tip[k])||!isfinite(m->position[k])||!isfinite(m->heading[k]))return 0;
 if(!isfinite(m->tracking_speed))return 0;
 SA3DSWhaleCommonResult result={0};
 memcpy(result.collision_center,i->tip,sizeof(result.collision_center));
 if(m->action==1){result.collision_center[0]=result.collision_center[2]=0;result.collision_center[1]=-100000;}
 if(i->kill_request==1)m->tracking_speed=2;
 else{
  float p[3];if(!s->buffered_position(s->context,p)||!isfinite(p[0])||!isfinite(p[2]))return 0;
  double dx=(double)p[0]-m->position[0],dz=(double)p[2]-m->position[2];
  /*The sum is stored to float before the source square-root call.*/
  double distance=sqrt((double)(float)(dx*dx+dz*dz));
  if(distance>400){if(m->tracking_speed<12)m->tracking_speed=(float)((double)m->tracking_speed+.1f);}
  else if(distance>200)m->tracking_speed=(float)((double)m->tracking_speed+(m->tracking_speed<6?(double).1f:-(double).1f));
  else if(m->tracking_speed>1)m->tracking_speed=(float)((double)m->tracking_speed-.1f);
  else m->tracking_speed=1;
 }
 uint16_t angle=(uint16_t)((uint32_t)m->yaw-(uint32_t)m->base_yaw);
 double bend=s->sine(s->context,angle);if(!isfinite(bend))return 0;
 float forward_x=0,forward_z=0;double scalar,zdelta;
 if(m->tracking_speed!=0){
  forward_x=(float)((double)m->heading[0]*m->tracking_speed);
  forward_z=(float)((double)m->heading[2]*m->tracking_speed);
  scalar=((double)1.8f/m->tracking_speed)*bend;
  result.movement[0]=(float)(scalar*forward_z+forward_x);
  zdelta=-(double)(float)scalar*forward_x+forward_z;
 }else{
  /*Reachable with a negative injected speed: retail has a divide-free branch.*/
  scalar=bend*(double)1.8f;
  result.movement[0]=(float)(scalar*m->heading[2]);
  zdelta=-(double)(float)scalar*m->heading[0];
 }
 result.movement[2]=(float)zdelta;
 m->position[0]=(float)((double)m->position[0]+result.movement[0]);
 /*FST (not FSTP) stores shared Z, then adds the still-extended value to posZ.*/
 m->position[2]=(float)((double)m->position[2]+zdelta);
 if(i->collision_flag8&&s->collision_character(s->context)==0){
  float velocity[3]={(float)((double)m->heading[0]*8),2,(float)((double)m->heading[2]*8)};
  s->bounce_player0(s->context,velocity);result.bounced=1;
 }
 s->enqueue_collision(s->context,result.collision_center);
 m->counter++;
 if(m->counter>18&&m->action!=2){
  uint16_t rx=s->random15(s->context),rz=s->random15(s->context);
  float jitter_x=(float)(((double)rx/32768-.5)*250);
  float x=(float)((double)jitter_x+result.collision_center[0]);
  float z=(float)(((double)rz/32768-.5)*250+result.collision_center[2]);
  s->wave(s->context,x,z,0,1,13);m->counter=0;result.wave=1;
 }
 s->display(s->context);*out=result;return 1;
}
