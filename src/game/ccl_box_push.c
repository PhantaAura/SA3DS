#include "ccl_box_push.h"
#include <math.h>
#include <string.h>
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
int sa3ds_ccl_box_side_push(const SA3DSCheckpointSweep*b,SA1Vec3 sample,float*out_x,float*out_z){
 if(!b||!b->sine||!out_x||!out_z||!finite3(sample)||!finite3(b->box_center)||!isfinite(b->player_radius)||
    !isfinite(b->box_x)||!isfinite(b->box_z))return 0;
 float sn=(float)b->sine(b->context,b->box_yaw),cs=(float)b->sine(b->context,(uint16_t)(b->box_yaw+16384));
 if(!isfinite(sn)||!isfinite(cs))return 0;
 /*419900: world->box local with the same yaw frame as4199D0's corners.*/
 float dx=(float)((double)sample.x-b->box_center.x),dz=(float)((double)sample.z-b->box_center.z);
 float lx=(float)((double)dx*cs-(double)dz*sn),lz=(float)((double)dx*sn+(double)dz*cs);
 float r=b->player_radius;
 float ex=(float)((double)b->box_x+r),ez=(float)((double)b->box_z+r);
 float a=(float)((double)ex*lz),c=(float)((double)ez*lx);
 double diagonal=(double)c+a; /*kept extended until its compare*/
 if((double)a-c<=0){
  if(diagonal>0)lx=(float)((double)r*.999f+b->box_x);
  else lz=(float)((double)ez*-.999f);
 }else{
  if(diagonal<=0)lx=(float)((double)ex*-.999f);
  else lz=(float)((double)ez*.999f);
 }
 /*419970: local->world, then 419B50's (w-s)+s writeback.*/
 float wx=(float)((double)b->box_center.x+(float)((double)lx*cs+(double)lz*sn));
 float wz=(float)((double)b->box_center.z+(float)((double)lz*cs-(double)lx*sn));
 *out_x=(float)(((double)wx-sample.x)+sample.x);*out_z=(float)(((double)wz-sample.z)+sample.z);
 return isfinite(*out_x)&&isfinite(*out_z);
}
int sa3ds_ccl_box_push(const SA3DSCheckpointSweep*b,const SA3DSCheckpointContact*hit,SA1Vec3 task,SA3DSBoxPushResult*out){
 if(!out)return 0;
 memset(out,0,sizeof(*out));
 if(!b||!hit||hit->kind>3||!finite3(task))return 0;
 out->position=task;
 if(!hit->kind)return 1;
 float upper=(float)((double)b->box_center.y+b->box_y),lower=(float)((double)b->box_center.y-b->box_y);
 if(hit->kind==1){
  out->position.y=(float)((double)upper-.01f);out->zero_tilt=1;out->status_clear=8;out->status_set=0x20;
 }else if(hit->kind==3){
  out->position.y=(float)(((double)lower+.01f)-(double)(float)((double)b->player_half_height+b->player_half_height));
  out->status_clear=8;out->status_set=0x10;
 }else if(!sa3ds_ccl_box_side_push(b,hit->sample,&out->position.x,&out->position.z))return 0;
 out->moved=1;return 1;
}
int sa3ds_ccl_box_push_enabled(const SA3DSCCLBody*p,const SA3DSCCLShape*primary,const SA3DSCCLBody*c,const SA3DSCCLShape*box){
 return p&&primary&&c&&box&&(p->rules.push_receive&box->push)&&
  !((primary->attributes&0x4000)&&(box->attributes&0x4000)&&(box->damage&0x10))&&
  !(c->rules.push_inhibit&primary->push);
}
