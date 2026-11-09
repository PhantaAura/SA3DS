#include "ccl_cylinder.h"
#include <math.h>
static int pushes(const SA3DSCCLBody*self,const SA3DSCCLShape*own,const SA3DSCCLBody*other,const SA3DSCCLShape*theirs){ /*418D80*/
 return (self->rules.push_receive&theirs->push)&&
  !((own->attributes&0x4000)&&(theirs->attributes&0x4000)&&(theirs->damage&0x10))&&
  !(other->rules.push_inhibit&own->push);
}
int sa3ds_ccl_cylinder_contact(SA3DSCCLBody*a,SA3DSCCLShape*as,unsigned ai,const SA3DSCCLPrimaryExtent*ext,
 SA3DSCCLBody*b,SA3DSCCLShape*bs,unsigned bi,float bh,int player0,SA1Vec3*pos,int*clear_tilt){
 if(!a||!as||!ext||!b||!bs||!pos||!clear_tilt||!isfinite(bh))return -1;
 *clear_tilt=0;
 float pa=as->radius,pb=ext->half_height,pc=ext->foot_radius,ca=bs->radius;
 float px=pos->x,py=(float)((double)pos->y+pb),pz=pos->z;             /*41C5D0 world-Y*/
 float cx=bs->center.x,cy=bs->center.y,cz=bs->center.z;               /*419270*/
 float d2=(float)((double)(pz-cz)*(pz-cz)+(double)(px-cx)*(px-cx));
 float ptop=(float)((double)py+pb),pbot=(float)((double)py-pb),ctop=(float)((double)cy+bh),cbot=(float)((double)cy-bh);
 float split=(float)((double)ptop*.4f+(double)pbot*.6f),foot=(float)((double)pc+ca);
 if(pbot<ctop&&ctop<split&&d2<foot*foot){
  sa3ds_ccl_record_contact(a,as,ai,b,bs,bi);
  a->outer_flags|=8;b->outer_flags|=2;
  if(pushes(a,as,b,bs)){
   if(player0)a->status|=0x20;
   b->status|=0x10;*clear_tilt=1;
   pos->y=(float)((double)(float)((double)ctop-.01f-pbot)+pos->y);
  }
  return 1;
 }
 float rr=(float)((double)ca+pa);
 if(!(d2<rr*rr))return 0;
 if(ptop<=cbot||(float)((double)cbot+2.f)<=ptop){
  if(!(pbot<ctop&&cbot<ptop&&split<ctop))return 0;
  sa3ds_ccl_record_contact(a,as,ai,b,bs,bi);
  a->outer_flags|=4;b->outer_flags|=4;
  float nx=(float)((double)px-cx),nz=(float)((double)pz-cz);
  double len=sqrt((double)nx*nx+(double)nz*nz);
  float depth=(float)((float)(len-pa)-ca);
  if(!(depth<0))return 2;
  if((float)len==0){nx=0;nz=.1f;}else{nx=(float)(nx/len);nz=(float)(nz/len);}
  a->direction_x=nx;a->direction_z=nz;
  if(pushes(a,as,b,bs)){
   float k=(float)((double)-depth-(double).1f);
   pos->x=(float)((double)(float)((double)nx*k)*(double).5f+pos->x);
   pos->z=(float)((double)(float)((double)k*nz)*(double).5f+pos->z);
  }
  return 2;
 }
 sa3ds_ccl_record_contact(a,as,ai,b,bs,bi);
 a->outer_flags|=2;b->outer_flags|=8;
 if(pushes(a,as,b,bs)){
  if(player0)a->status|=0x10;
  b->status|=0x20;
  pos->y=(float)((double)(float)((double)cbot+.01f-ptop)+pos->y);
 }
 return 3;
}
