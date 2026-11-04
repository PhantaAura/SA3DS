#include "whale_draw.h"
#include <math.h>
#include <string.h>
static int32_t ri(const unsigned char*p){uint32_t u=p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);int32_t i;memcpy(&i,&u,4);return i;}
static void identity(SA3CMatrix*m){memset(m,0,sizeof(*m));for(unsigned i=0;i<4;i++)m->m[i*5]=1;}
static int rotate(SA3CMatrix*m,unsigned axis,int32_t angle,const SA3DSWhaleMotionServices*s){
 if(!angle)return 1;
 double sn=s->sine(s->context,(uint16_t)angle),cs=s->sine(s->context,(uint16_t)((uint32_t)angle+0x4000));
 if(!isfinite(sn)||!isfinite(cs))return 0;
 float sf=(float)sn;unsigned a=axis==0?1:axis==1?0:0,b=axis==0?2:axis==1?2:1;
 double sign=axis==1?-1:1;
 for(unsigned row=0;row<4;row++){
  float first=m->m[a*4+row],second=m->m[b*4+row];
  m->m[a*4+row]=(float)((double)first*cs+sign*(double)second*sf);
  m->m[b*4+row]=(float)((double)second*cs-sign*(double)first*sf);
 }
 return 1;
}
int sa3ds_whale_cycle_pitch(const SA3DSWhaleTipAssets*a,float frame,int32_t*out){
 if(!a||!a->pitch||!out||!isfinite(frame)||frame<0||frame>=4294967296.0)return 0;
 frame=(float)((double)frame-((uint32_t)frame/32)*32.0);
 unsigned first=(unsigned)frame,second=(first+1)%32;
 /*404C10 uses denominator1 when next key-frame difference is not positive.
  *Unlike shape4054C0, the final signed difference is NOT a huge divisor.*/
 double weight=(double)frame-first;
 volatile double left=(double)ri(a->pitch+first*4)*(1-weight),right=(double)ri(a->pitch+second*4)*weight;
 *out=(int32_t)(left+right);return 1;
}
int sa3ds_whale_draw_transform(const SA3DSWhaleMotion*m,int32_t pitch,
 const SA3DSWhaleMotionServices*s,SA3CMatrix*out,unsigned*bank){
 if(!m||!s||!s->sine||!out||!bank||!isfinite(m->draw_y))return 0;
 SA3CMatrix actor;identity(&actor);
 for(unsigned i=0;i<3;i++){if(!isfinite(m->position[i]))return 0;actor.m[12+i]=m->position[i];}
 actor.m[13]=(float)((double)m->position[1]+m->draw_y);
 if(!rotate(&actor,1,m->yaw,s)||!rotate(&actor,2,m->roll,s)||!rotate(&actor,0,pitch,s))return 0;
 *out=actor;*bank=m->action==2?1:0;return 1;
}
