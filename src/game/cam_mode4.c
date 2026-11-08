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
/*462B40: keep the new eye unless the move from the old camera hits.*/
static void final_ray(float eye[3],const float old[3],const float p[3],int turning,const SA3DSCamServices*sv){
 if(!sv->ray)return;
 float start[3];memcpy(start,old,12);
 float delta[3]={(float)((double)eye[0]-old[0]),(float)((double)eye[1]-old[1]),(float)((double)eye[2]-old[2])};
 float po[3]={p[0]-old[0],p[1]-old[1],p[2]-old[2]},pe[3]={p[0]-eye[0],p[1]-eye[1],p[2]-eye[2]};
 double lo=(float)sqrt((double)po[0]*po[0]+(double)po[1]*po[1]+(double)po[2]*po[2]);
 double le=sqrt((double)pe[0]*pe[0]+(double)pe[1]*pe[1]+(double)pe[2]*pe[2]);
 if(sv->ray(sv->context,start,delta,le<=lo?4.f:8.f)){
  if(!turning)for(unsigned a=0;a<3;a++)eye[a]=delta[a]+start[a];
  else for(unsigned a=0;a<3;a++)eye[a]=(float)(((double)eye[a]-start[a])*(double).1f+start[a]);
 }
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
 final_ray(eye,old,p,in->turn_left||in->turn_right,sv);
 sa3ds_cam_apply_look(c); /*437A20 -> 435C70 for mode byte2*/
 return finite3(c->position);
}
int sa3ds_cam_mode20_update(SA3DSCamera*c,const SA3DSCamInput*in,const float eye[3],uint8_t still,uint8_t allowed,
 uint8_t*manual,const SA3DSCamServices*sv,int*adjust){
 if(!c||!in||!eye||!manual||!sv||!adjust||!finite3(in->player)||!finite3(eye))return 0;
 *adjust=-1;
 if(in->frames_since_reset==0)*manual=0;                     /*46A363*/
 if(*manual){
  if(!sa3ds_cam_mode4_update(c,in,sv))return 0;              /*46A393*/
  if(!in->turn_left&&!in->turn_right&&!still){*manual=0;*adjust=0x1a;}
  return 1;
 }
 if((in->turn_left||in->turn_right)&&allowed)*manual=1;      /*46A42E: placement still runs*/
 memcpy(c->param.eye,eye,12);
 c->param.target[0]=in->player[0];c->param.target[1]=(float)((double)in->player[1]+7.f);c->param.target[2]=in->player[2];
 sa3ds_cam_apply_look(c);
 return finite3(c->position);
}
/*418AC0-style normalize; returns the length (zero vector left zero).*/
static double normalize3(float v[3]){
 double l=sqrt((double)v[0]*v[0]+(double)v[1]*v[1]+(double)v[2]*v[2]);
 if(l!=0){for(unsigned k=0;k<3;k++)v[k]=(float)(v[k]/l);}
 return l;
}
static int lr(const SA3DSCamInput*in){return in->turn_left||in->turn_right;}
/*Shared manual tail (46A049/46A230/46A392): mode4, then release.*/
static int manual_tail(SA3DSCamera*c,const SA3DSCamInput*in,const SA3DSCamExtra*x,uint8_t*manual,const SA3DSCamServices*sv,int*adjust){
 if(!sa3ds_cam_mode4_update(c,in,sv))return 0;
 if(!lr(in)&&!x->player_still){*manual=0;*adjust=0x1a;}
 return 1;
}
int sa3ds_cam_orbit_update(SA3DSCamera*c,const SA3DSCamInput*in,const SA3DSCamExtra*x,uint8_t*orbit,const SA3DSCamServices*sv,int*adjust){
 if(!x->stick_active&&!x->buttons_active&&x->player_still){
  int start=lr(in)&&x->adjust_index==0;
  if(start)*orbit=1;
  if(*orbit){
   float*e=c->param.eye,*t=c->param.target;
   float dx=(float)((double)t[0]-e[0]),dz=(float)((double)t[2]-e[2]);
   double h=(float)sqrt((float)((double)dz*dz+(double)dx*dx));
   float dy=(float)((double)e[1]-t[1]);
   int32_t a=(int32_t)(atan2(dx,dz)*65536.0*0.15915497620314795);
   int32_t d=(int32_t)((float)(((int32_t)x->analog_r-(int32_t)x->analog_l)<<8)*-0.005687499884516001f);
   int32_t ang=a-d+0x8000;
   e[0]=(float)(sv->sine(sv->context,(uint16_t)ang)*h+t[0]);
   e[1]=(float)((double)dy+t[1]);
   e[2]=(float)(sv->sine(sv->context,(uint16_t)(ang+0x4000))*h+t[2]);
   return 1;
  }
  return 0;
 }
 if(*orbit==1)*adjust=8;
 *orbit=0;return 0;
}
int sa3ds_cam_mode39_update(SA3DSCamera*c,const SA3DSCamInput*in,const SA3DSCamExtra*x,const SA3DSCamRecord*r,
 SA3DSCamModeState*st,const SA3DSCamServices*sv,int*adjust){
 if(!c||!in||!x||!r||!st||!sv||!sv->sine||!adjust||!finite3(in->player))return 0;
 *adjust=-1;
 if(in->frames_since_reset==0)st->manual39=0;
 else if(st->manual39)return manual_tail(c,in,x,&st->manual39,sv,adjust);
 if(lr(in)&&x->manual_allowed&&!in->manual_disabled)st->manual39=1;   /*469EF7*/
 float old[3];memcpy(old,c->position,12);
 float*eye=c->param.eye,*tgt=c->param.target;const float*p=in->player;
 if(!sa3ds_cam_orbit_update(c,in,x,&st->orbit,sv,adjust)){
  tgt[0]=p[0];tgt[1]=(float)((double)p[1]+7.5f);tgt[2]=p[2];
  float d[3]={(float)((double)p[0]-r->point[0]),(float)((double)p[1]-r->point[1]),(float)((double)p[2]-r->point[2])};
  normalize3(d);
  for(unsigned k=0;k<3;k++)eye[k]=(float)((double)d[k]*r->extent3c+tgt[k]);
 }
 if(sv->regions)sv->regions(sv->context,eye,old);
 /*436C60: 03B2CBBC is never set in the shipped exe, so no blend.*/
 if(r->word20==0)final_ray(eye,old,p,lr(in),sv);
 sa3ds_cam_apply_look(c);
 return finite3(c->position);
}
int sa3ds_cam_mode31_update(SA3DSCamera*c,const SA3DSCamInput*in,const SA3DSCamExtra*x,const SA3DSCamRecord*r,
 SA3DSCamModeState*st,const SA3DSCamServices*sv,int*adjust){
 if(!c||!in||!x||!r||!st||!sv||!adjust||!finite3(in->player))return 0;
 *adjust=-1;
 if(in->frames_since_reset==0){
  st->height31=r->word20?(float)((float)r->word20*0.1f):(float)(r->extent3c*0.5f);
  st->drift31=0;st->manual31=0;
 }else if(st->manual31)return manual_tail(c,in,x,&st->manual31,sv,adjust);
 if(lr(in)&&x->manual_allowed)st->manual31=1;                         /*46A127*/
 float old[3];memcpy(old,c->position,12);
 float*eye=c->param.eye,*tgt=c->param.target;const float*p=in->player;
 memcpy(tgt,p,12);
 float d[3]={(float)((double)p[0]-r->point[0]),0,(float)((double)p[2]-r->point[2])};
 normalize3(d);
 eye[0]=(float)((double)d[0]*r->extent3c+tgt[0]);
 eye[1]=(float)((double)st->height31+tgt[1]);
 eye[2]=(float)((double)d[2]*r->extent3c+tgt[2]);
 if(x->airborne&&r->word22==0)st->drift31=(float)((double)st->drift31-0.2f);
 else st->drift31=(float)((double)st->drift31*0.9f);
 eye[1]=(float)((double)eye[1]-st->drift31);
 float v[3]={(float)((double)eye[0]-r->point[0]),0,(float)((double)eye[2]-r->point[2])};
 double l=normalize3(v);
 if(l!=0)eye[1]=(float)((l-r->extent3c)/l*st->height31+eye[1]);
 if(sv->regions)sv->regions(sv->context,eye,old);
 final_ray(eye,old,p,lr(in),sv);
 sa3ds_cam_apply_look(c);
 return finite3(c->position);
}
