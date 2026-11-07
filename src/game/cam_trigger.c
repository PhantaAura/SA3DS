#include "cam_trigger.h"
#include <math.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)(p[0]|(p[1]<<8));}
static float rf(const uint8_t*p){uint32_t u=p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);float f;memcpy(&f,&u,4);return f;}
int sa3ds_cam_record_decode(SA3DSCamRecord*o,const uint8_t*r){
 if(!o||!r)return 0;
 SA3DSCamRecord n={r[0],r[1],r[2],r[3],r16(r+4),r16(r+6),{rf(r+8),rf(r+12),rf(r+16)},{rf(r+20),rf(r+24),rf(r+28)}};
 *o=n;return 1;
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
