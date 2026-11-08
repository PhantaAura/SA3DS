#include "cam_trigger.h"
#include <math.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)(p[0]|(p[1]<<8));}
static float rf(const uint8_t*p){uint32_t u=p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);float f;memcpy(&f,&u,4);return f;}
int sa3ds_cam_record_decode(SA3DSCamRecord*o,const uint8_t r[64]){
 if(!o||!r)return 0;
 SA3DSCamRecord n={r[0],r[1],r[2],r[3],r16(r+4),r16(r+6),{rf(r+8),rf(r+12),rf(r+16)},{rf(r+20),rf(r+24),rf(r+28)},{rf(r+36),rf(r+40),rf(r+44)},{rf(r+48),rf(r+52),rf(r+56)},r16(r+32),r16(r+34),rf(r+60)};
 *o=n;return 1;
}
int sa3ds_cam_records_decode(SA3DSCamRecord*out,uint32_t cap,const uint8_t*rows,size_t size,uint32_t*count){
 if(!out||!rows||!count||size%64u||size/64u>cap)return 0;
 uint32_t n=(uint32_t)(size/64u);for(uint32_t i=0;i<n;i++)sa3ds_cam_record_decode(&out[i],rows+(size_t)i*64u);
 *count=n;return 1;
}
/*Inverse of T(center)*Rx*Ry (784BE0, 784EE0, 784E20, then 787630): world point
 *-> record-local. Ninja rotations follow SA Tools' element order.*/
static void to_local(const SA3DSCamRecord*r,double sx,double cx,double sy,double cy,const float p[3],double out[3]){
 /*Columns of nj_rotx(x) then nj_roty(y) applied to the identity (sa3c order).*/
 double c0[3]={cy,sy*sx,-sy*cx};
 double c1[3]={0,cx,sx};
 double c2[3]={sy,-cy*sx,cy*cx};
 double d[3]={(double)p[0]-r->center[0],(double)p[1]-r->center[1],(double)p[2]-r->center[2]};
 out[0]=c0[0]*d[0]+c0[1]*d[1]+c0[2]*d[2];
 out[1]=c1[0]*d[0]+c1[1]*d[1]+c1[2]*d[2];
 out[2]=c2[0]*d[0]+c2[1]*d[1]+c2[2]*d[2];
}
int sa3ds_cam_trigger_inside(const SA3DSCamRecord*r,const float cur[3],const float h1[3],const float h2[3],
 double(*sine)(void*,uint16_t),void*ctx,int*inside){
 if(!r||!cur||!inside)return 0;
 *inside=0;
 double dx=(double)cur[0]-r->center[0],dy=((double)cur[1]-r->center[1])+4.,dz=(double)cur[2]-r->center[2];
 double dist=(float)(dx*dx+dy*dy+dz*dz);
 if(r->shape==0){*inside=dist<r->extent[2];return 1;}          /*4357F0 strict*/
 if(r->shape!=1&&r->shape!=2)return 1;                          /*unknown shapes never trigger*/
 if(!h1||!sine||(r->shape==1&&!h2))return 0;
 double ex=r->extent[0],ey=r->extent[1],ez=r->extent[2];
 double broad=r->shape==1?(float)(ex*ex+ey*ey):(float)(ex*ex+ey*ey+ez*ez);
 if(!(dist<=broad))return 1;
 double sx=sine(ctx,r->rot_x),cx=sine(ctx,(uint16_t)(r->rot_x+16384)),sy=sine(ctx,r->rot_y),cy=sine(ctx,(uint16_t)(r->rot_y+16384));
 if(r->rot_x==0){sx=0;cx=1;}if(r->rot_y==0){sy=0;cy=1;}         /*source skips zero rotations*/
 float p1[3]={h1[0],(float)((double)h1[1]+4.),h1[2]};double a[3];to_local(r,sx,cx,sy,cy,p1,a);
 if(r->shape==2){                                                /*435AA0 inclusive box*/
  *inside=a[0]<=ex&&-ex<=a[0]&&a[1]<=ey&&-ey<=a[1]&&a[2]<=ez&&-ez<=a[2];return 1;
 }
 float p2[3]={h2[0],(float)((double)h2[1]+4.),h2[2]};double b[3];to_local(r,sx,cx,sy,cy,p2,b);
 /*435840: previous(selector2) and current(selector1) must straddle local z=0.*/
 if(!((0<=b[2]||0<=a[2])&&(b[2]<=0||a[2]<=0)))return 1;
 double dir[3]={a[0]-b[0],a[1]-b[1],a[2]-b[2]},hx,hy;
 if(dir[2]==0){hx=b[0];hy=b[1];}                                 /*parallel branch (795770/788D40) not decoded*/
 else{double t=-b[2]/dir[2];hx=b[0]+t*dir[0];hy=b[1]+t*dir[1];}  /*788640, plane z=0*/
 *inside=hx<=ex&&-ex<=hx&&hy<=ey&&-ey<=hy;return 1;
}

static void rotation(const SA3DSCamRecord*r,double(*sine)(void*,uint16_t),void*ctx,double*sx,double*cx,double*sy,double*cy){
 *sx=0;*cx=1;*sy=0;*cy=1;
 if(r->rot_x){*sx=sine(ctx,r->rot_x);*cx=sine(ctx,(uint16_t)(r->rot_x+16384));}
 if(r->rot_y){*sy=sine(ctx,r->rot_y);*cy=sine(ctx,(uint16_t)(r->rot_y+16384));}
}
int sa3ds_cam_region_sphere_push(const SA3DSCamRecord*r,float eye[3]){
 if(!r||!eye)return 0;
 float d[3]={(float)((double)eye[0]-r->center[0]),(float)((double)eye[1]-r->center[1]),(float)((double)eye[2]-r->center[2])};
 double q=(double)d[0]*d[0]+(double)d[1]*d[1]+(double)d[2]*d[2];
 if(!(q<r->extent[2]))return 0;
 double l=sqrt(q);if(l!=0){for(unsigned k=0;k<3;k++)d[k]=(float)(d[k]/l);}  /*418AC0*/
 for(unsigned k=0;k<3;k++)eye[k]=(float)((double)d[k]*r->extent[0]+r->center[k]);
 return 1;
}
int sa3ds_cam_region_box_push(const SA3DSCamRecord*r,float eye[3],double(*sine)(void*,uint16_t),void*ctx){
 if(!r||!eye||!sine)return 0;
 double sx,cx,sy,cy;rotation(r,sine,ctx,&sx,&cx,&sy,&cy);
 double l[3];to_local(r,sx,cx,sy,cy,eye,l);
 float lx=(float)l[0],ly=(float)l[1],lz=(float)l[2];
 float ax=fabsf(lx),ay=fabsf(ly),az=fabsf(lz);
 if(!(ax<=r->extent[0])||!(ay<=r->extent[1])||!(az<=r->extent[2]))return 0;
 float gx=r->extent[0]-ax,gy=r->extent[1]-ay,gz=r->extent[2]-az;
 int axis=0;
 if(gy<gx){axis=1;if(gz<gy)axis=2;}else if(gz<gx)axis=2;
 float*c=axis==0?&lx:axis==1?&ly:&lz;float e=r->extent[axis];
 *c=*c<=0?-e:e;
 double c0[3]={cy,sy*sx,-sy*cx},c1[3]={0,cx,sx},c2[3]={sy,-cy*sx,cy*cx};
 for(unsigned k=0;k<3;k++)eye[k]=(float)(c0[k]*lx+c1[k]*ly+c2[k]*lz+r->center[k]);
 return 1;
}
int sa3ds_cam_region_gate_push(const SA3DSCamRecord*r,const float old[3],float eye[3],double(*sine)(void*,uint16_t),void*ctx){
 if(!r||!old||!eye||!sine)return 0;
 double dx=(double)r->center[0]-eye[0],dy=(double)r->center[1]-eye[1],dz=(double)r->center[2]-eye[2];
 double ex=r->extent[0],ey=r->extent[1];
 if(ex*ex+ey*ey<dx*dx+dy*dy+dz*dz)return 0;
 double a[3]={(double)old[0]-r->point[0],(double)old[1]-r->point[1],(double)old[2]-r->point[2]};
 double b[3]={(double)eye[0]-r->point[0],(double)eye[1]-r->point[1],(double)eye[2]-r->point[2]};
 double la=(float)sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]),lb=(float)sqrt(b[0]*b[0]+b[1]*b[1]+b[2]*b[2]);
 if(lb<la)return 0;                                             /*moving toward +24*/
 double sx,cx,sy,cy;rotation(r,sine,ctx,&sx,&cx,&sy,&cy);
 double lo[3],le[3];to_local(r,sx,cx,sy,cy,old,lo);to_local(r,sx,cx,sy,cy,eye,le);
 float oz=(float)lo[2],ez=(float)le[2];
 int cross=(oz<0)?(ez>=0):(oz<=0||ez<=0);                       /*4369B6..436A14*/
 if(!cross)return 0;
 /*788640 against the zero plane: NaN hit, every rectangle compare passes.*/
 memcpy(eye,old,12);return 1;
}
int sa3ds_cam_regions_apply(const SA3DSCamRecord*records,uint32_t count,const uint8_t prio[256],
 const float old[3],float eye[3],double(*sine)(void*,uint16_t),void*ctx,unsigned*gate_reverts){
 if(gate_reverts)*gate_reverts=0;
 if(!records||!prio||!old||!eye||count<2)return 0;
 int pushes=0;
 for(uint32_t i=count-1;i>=1;i--){                              /*437E90*/
  const SA3DSCamRecord*r=&records[i];if(prio[r->mode]!=4)continue;
  if(r->shape==0)pushes+=sa3ds_cam_region_sphere_push(r,eye);
  else if(r->shape==2)pushes+=sa3ds_cam_region_box_push(r,eye,sine,ctx);
 }
 for(uint32_t i=count-1;i>=1;i--){                              /*437F20*/
  const SA3DSCamRecord*r=&records[i];if(prio[r->mode]!=4||r->shape!=1)continue;
  if(sa3ds_cam_region_gate_push(r,old,eye,sine,ctx)){pushes++;if(gate_reverts)(*gate_reverts)++;}
 }
 return pushes;
}
