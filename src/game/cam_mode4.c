#include "cam_mode4.h"
#include <math.h>
#include <string.h>
#define TO_BAMS (65536.0*0.15915497620314795)
static int finite3(const float p[3]){return isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2]);}
/*438350: step angle a toward b by at most step (signed16 difference).*/
static uint16_t approach(uint32_t a,uint32_t b,int step){
 uint32_t ua=a&0xffff,ub=b&0xffff;int diff=(int16_t)(ub-ua);
 if(step<diff||diff< -step){
  if((int8_t)((ub-ua)>>8)<0)return (uint16_t)(ua-step);
  return (uint16_t)(ua+step);
 }
 return (uint16_t)ub;
}
void sa3ds_cam_apply_look(SA3DSCamera*c){
 memcpy(c->position,c->param.eye,12);
 double dx=(float)((double)c->param.eye[0]-c->param.target[0]),dz=(float)((double)c->param.eye[2]-c->param.target[2]);
 c->rot_y=(int32_t)(atan2(dx,dz)*TO_BAMS);
 double horizontal=sqrt((float)(dz*dz+dx*dx));
 double dy=(float)((double)c->param.target[1]-c->param.eye[1]);
 c->rot_x=(int32_t)(atan2(dy,horizontal)*TO_BAMS);
 c->rot_z=c->param.roll;
}
int sa3ds_cam_mode4_update(SA3DSCamera*c,const SA3DSCamInput*in,const SA3DSCamServices*sv){
 if(!c||!in||!sv||!sv->sine||!finite3(in->player)||!finite3(in->velocity))return 0;
 float*eye=c->param.eye,*tgt=c->param.target;const float*p=in->player;
 float old[3];memcpy(old,c->position,12); /*Camera_Data1 position on entry*/
 /*469598: a fresh mode seeds the target at the player.*/
 if(in->frames_since_reset==0)memcpy(tgt,p,12);
 /*469614..469696: look-ahead goal, eased by .1 per axis.*/
 float goal[3]={(float)((double)in->velocity[0]*8.f+p[0]),
  (float)((double)in->velocity[1]*8.f+(double)(float)((double)p[1]+in->eyes_height)),
  (float)((double)in->velocity[2]*8.f+p[2])};
 for(unsigned k=0;k<3;k++)tgt[k]=(float)(((double)goal[k]-tgt[k])*(double).1f+tgt[k]);
 /*4697F8: pitch toward F1C8 when off by more than 71C.*/
 int pd=(int16_t)((uint32_t)c->rot_x-0xf1c8u);if(pd<0)pd=-pd;
 if((int16_t)pd>0x71c){
  c->rot_x=approach((uint32_t)c->rot_x,0xf1c8u,0xb6);
  eye[1]=(float)((double)eye[1]+sv->sine(sv->context,(uint16_t)(c->rot_x+0xe38)));
 }
 /*46984E: manual turn, 2 units on the yaw circle.*/
 if(!in->manual_disabled){
  double cs=sv->sine(sv->context,(uint16_t)(c->rot_y+16384)),sn=sv->sine(sv->context,(uint16_t)c->rot_y);
  if(in->turn_left){eye[0]=(float)(cs*2+eye[0]);eye[2]=(float)(eye[2]-sn*2);}
  if(in->turn_right){eye[0]=(float)(eye[0]-cs*2);eye[2]=(float)(sn*2+eye[2]);}
 }
 /*469920..469AB7: distance rules along the player-to-eye direction.*/
 float dir[3]={(float)((double)p[0]-eye[0]),(float)((double)p[1]-eye[1]),(float)((double)p[2]-eye[2])};
 double dist=sqrt((double)dir[0]*dir[0]+(double)dir[1]*dir[1]+(double)dir[2]*dir[2]);
 if(dist!=0){float k=(float)(1./dist);for(unsigned a=0;a<3;a++)dir[a]*=k;}else dir[0]=dir[1]=dir[2]=0;
 double d=dist;
 if(d>60.){
  d=(d-60.)*(double).9f+60.;
  if(d>90.){
   d=90.;
   if(sv->ray){ /*4370C0 probe from the camera toward the player*/
    float start[3];memcpy(start,old,12);
    float v[3]={(float)((double)p[0]-old[0]),(float)((double)p[1]-old[1]),(float)((double)p[2]-old[2])};
    double l=sqrt((double)v[0]*v[0]+(double)v[1]*v[1]+(double)v[2]*v[2]);
    if(l!=0){float k=(float)(1./l);for(unsigned a=0;a<3;a++)v[a]*=k;}else v[0]=v[1]=v[2]=0;
    v[0]*=10.f;v[1]*=20.f;v[2]*=10.f;
    if(sv->ray(sv->context,start,v,4.f)){
     double vl=sqrt((double)v[0]*v[0]+(double)v[1]*v[1]+(double)v[2]*v[2]);
     if(vl<3.)d=20.;
    }
   }
  }
 }else if(d<35.){
  d=d*(double)1.06f;if(!(d>20.))d=20.;
 }
 for(unsigned a=0;a<3;a++)eye[a]=(float)((double)p[a]-dir[a]*d);
 /*469ABA: an eye above the player keeps30 units horizontally.*/
 if(eye[1]>p[1]){
  float hx=(float)((double)eye[0]-p[0]),hz=(float)((double)eye[2]-p[2]);
  if((double)hz*hz+(double)hx*hx<900.){
   double l=sqrt((double)hx*hx+(double)hz*hz);
   if(l!=0){hx=(float)(hx/l);hz=(float)(hz/l);}else hx=hz=0;
   eye[0]=(float)((double)hx*30.f+p[0]);eye[2]=(float)((double)hz*30.f+p[2]);
  }
 }
 if(sv->regions)sv->regions(sv->context,eye,old);
 /*462B40: keep the new eye unless the move from the old camera hits.*/
 if(sv->ray){
  float start[3];memcpy(start,old,12);
  float delta[3]={(float)((double)eye[0]-old[0]),(float)((double)eye[1]-old[1]),(float)((double)eye[2]-old[2])};
  float po[3]={p[0]-old[0],p[1]-old[1],p[2]-old[2]},pe[3]={p[0]-eye[0],p[1]-eye[1],p[2]-eye[2]};
  double lo=(float)sqrt((double)po[0]*po[0]+(double)po[1]*po[1]+(double)po[2]*po[2]);
  double le=sqrt((double)pe[0]*pe[0]+(double)pe[1]*pe[1]+(double)pe[2]*pe[2]);
  if(sv->ray(sv->context,start,delta,le<=lo?4.f:8.f)){
   if(!in->turn_left&&!in->turn_right)for(unsigned a=0;a<3;a++)eye[a]=delta[a]+start[a];
   else for(unsigned a=0;a<3;a++)eye[a]=(float)(((double)eye[a]-start[a])*(double).1f+start[a]);
  }
 }
 sa3ds_cam_apply_look(c); /*437A20 -> 435C70 for mode byte2*/
 return finite3(c->position);
}
