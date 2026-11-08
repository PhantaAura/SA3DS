#include "cam_adjust.h"
#include <math.h>
#include <string.h>
#define K (65536.0*0.15915497620314795)
static const struct {uint8_t ray;float duration;int32_t frames;} TABLE[21]={
 {0,60,0},{1,60,0},{0,180,3},{1,180,3},{0,600,10},{1,600,10},{0,1800,30},{1,1800,30},{0,3600,60},{1,3600,60},
 {0,60,0},{1,60,0},{0,180,3},{1,180,3},{0,600,10},{1,600,10},{0,1800,30},{1,1800,30},{0,3600,60},{1,3600,60},{1,30000,60}};
static int32_t absdiff(int32_t a,int32_t b){int32_t d=(int16_t)(uint16_t)(b-a);if(d<0)d=-d;return d&0xffff;} /*4383B0*/
static int32_t approach(int32_t a,int32_t b,int32_t step){                  /*438350*/
 uint32_t ua=(uint32_t)a&0xffff,ub=(uint32_t)b&0xffff;int diff=(int16_t)(ub-ua);
 if(step<diff||diff< -step){
  if((int8_t)((ub-ua)>>8)<0)return (int32_t)(uint16_t)(ua-(uint32_t)step);
  return (int32_t)(uint16_t)(ua+(uint32_t)step);
 }
 return (int32_t)ub;
}
static int finite3(const float p[3]){return isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2]);}
/*462CF0 on positions: keep new unless old->new hits; turning keeps 10%.*/
static void position_ray(float n[3],const float o[3],const float p[3],int turning,const SA3DSCamServices*sv){
 if(!sv->ray)return;
 float delta[3]={(float)((double)n[0]-o[0]),(float)((double)n[1]-o[1]),(float)((double)n[2]-o[2])},start[3];memcpy(start,o,12);
 float po[3]={p[0]-o[0],p[1]-o[1],p[2]-o[2]},pn[3]={p[0]-n[0],p[1]-n[1],p[2]-n[2]};
 double lo=(float)sqrt((double)po[0]*po[0]+(double)po[1]*po[1]+(double)po[2]*po[2]);
 double ln=sqrt((double)pn[0]*pn[0]+(double)pn[1]*pn[1]+(double)pn[2]*pn[2]);
 if(!sv->ray(sv->context,start,delta,ln<=lo?4.f:8.f))return;
 if(!turning)for(unsigned k=0;k<3;k++)n[k]=(float)((double)delta[k]+o[k]);
 else for(unsigned k=0;k<3;k++)n[k]=(float)(((double)n[k]-o[k])*(double).1f+o[k]);
}
int sa3ds_cam_adjust_run(SA3DSCamera*n,const SA3DSCamera*o,SA3DSCamAdjust*a,uint8_t*idxp,uint8_t mode,
 const float p[3],const float prev[3],int turning,const SA3DSCamServices*sv){
 if(!n||!o||!a||!idxp||!p||!prev||!sv||!finite3(p)||!finite3(prev)||!finite3(o->position))return 0;
 uint8_t idx=*idxp;if(idx==0)return 1;                                      /*467D80*/
 if(idx<6||idx>26)return 0;
 /*468470: old camera's angles to the player.*/
 float dx=(float)((double)o->position[0]-p[0]),dz=(float)((double)o->position[2]-p[2]);
 int32_t ay=(int32_t)(atan2(dx,dz)*K);
 double h=(float)sqrt((float)((double)dx*dx+(double)dz*dz));
 int32_t ax=(int32_t)(atan2((float)((double)p[1]-o->position[1]),h)*K);
 if(a->counter==0){                                                          /*4683C0*/
  a->start_x=absdiff(o->rot_x,ax);a->start_y=absdiff(o->rot_y,ay);
  a->use_ray=TABLE[idx-6].ray;a->duration=TABLE[idx-6].duration;a->frames=TABLE[idx-6].frames;
 }
 float t=(float)((double)(int32_t)a->counter/a->duration);a->t=t;
 int finished=0;
 if(!(t<1.f)){t=1.f;a->t=1.f;*idxp=(uint8_t)(mode==0x4c?0xb:0);finished=1;}
 int32_t lim_x=(int32_t)((float)(0x1c71-a->start_x)*t+(float)a->start_x);
 int32_t lim_y=(int32_t)((float)(0xe38-a->start_y)*t+(float)a->start_y);
 float*np=n->position;const float*op=o->position;
 if(idx<=15){                                                                /*4684E0*/
  float s=(float)(1.0f-t),dp[3],v[3];
  for(unsigned k=0;k<3;k++){dp[k]=(float)((float)(((double)op[k]-p[k])*s)*(double).2f);v[k]=(float)((((double)np[k]-op[k])+dp[k])*t);}
  int32_t c=(int32_t)a->counter;
  if(c>=a->frames)for(unsigned k=0;k<3;k++)np[k]=(float)((double)v[k]+op[k]);
  else{float nf=(float)a->frames;for(unsigned k=0;k<3;k++)np[k]=(float)((double)c*v[k]/nf+op[k]);} /*(frames-c)*ABB8 term: ABB8..C0 are 0 during a blend*/
 }else{                                                                      /*468650*/
  for(unsigned k=0;k<3;k++){
   float off_old=(float)((double)op[k]-prev[k]),off_new=(float)((double)np[k]-p[k]);
   np[k]=(float)((double)(float)((double)(float)(((double)off_new-off_old)*t)+p[k])+off_old);
  }
 }
 /*4686F0: rotations eased from old by |d|*t.*/
 int32_t*nr[3]={&n->rot_x,&n->rot_y,&n->rot_z};const int32_t orr[3]={o->rot_x,o->rot_y,o->rot_z};
 for(unsigned k=0;k<3;k++){int32_t step=(int32_t)((float)absdiff(*nr[k],orr[k])*t);*nr[k]=approach(orr[k],*nr[k],step);}
 /*468790: stay within the limits of the old camera's look-at angles.*/
 if(absdiff(n->rot_x,ax)>lim_x)n->rot_x=approach(ax,n->rot_x,lim_x);
 if(absdiff(n->rot_y,ay)>lim_y)n->rot_y=approach(ay,n->rot_y,lim_y);
 if(a->use_ray)position_ray(np,op,p,turning,sv);
 a->counter=finished?0:a->counter+1;                                         /*436560 clears, 437B6A advances*/
 return finite3(np);
}
