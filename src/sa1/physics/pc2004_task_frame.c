#include "pc2004_task_frame.h"
#include <math.h>
#define TAU 6.283185307179586476925286766559
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
int sa1_pc2004_task_yaw(const SA1SurfaceFrame*f,uint16_t*yaw){
    if(!f||!yaw||!f->valid||!finite3(f->up)||!finite3(f->forward)||!finite3(f->right))return 0;
    double a=f->forward.z,b=-(double)f->right.z;
    /* At the normal pole the source canonical Z is zero; use the remaining
     * two forward components instead of dividing by the vanished cos(X). */
    if(hypot(a,b)<1e-6){a=f->up.z>=0?-(double)f->forward.y:f->forward.y;b=f->forward.x;}
    if(hypot(a,b)<1e-6)return 0;
    *yaw=(uint16_t)(int32_t)lround(atan2(a,b)*(65536./TAU));return 1;
}
int sa1_pc2004_task_turn(SA1SurfaceFrame*f,uint16_t old,uint16_t next,const SA1PC2004HomingMath*m){
    if(!f||!f->valid||!m||!m->sine)return 0;
    uint16_t delta=(uint16_t)(next-old);
    double s=m->sine(m->context,delta),c=m->sine(m->context,(uint16_t)(delta+0x4000));
    if(!isfinite(s)||!isfinite(c))return 0;
    SA1Vec3 forward={(float)(f->forward.x*c-f->right.x*s),(float)(f->forward.y*c-f->right.y*s),(float)(f->forward.z*c-f->right.z*s)};
    SA1SurfaceFrame next_frame=sa1_surface_frame_make(f->up,forward);
    if(!next_frame.valid)return 0;
    *f=next_frame;return 1;
}

static uint16_t toward_zero(uint16_t u){
    int d=(int16_t)(uint16_t)(0-u);
    return d>0x800?(uint16_t)(u+0x800):d< -0x800?(uint16_t)(u-0x800):0;
}
static double frame_sine(const SA1PC2004HomingMath*m,uint16_t a){return m&&m->sine?m->sine(m->context,a):sin(a*(TAU/65536.));}
int sa1_pc2004_task_relax_up(SA1SurfaceFrame*f,const SA1PC2004HomingMath*m){
    uint16_t y;if(!sa1_pc2004_task_yaw(f,&y))return 0;
    double nz=fmax(-1.,fmin(1.,f->up.z));
    uint16_t x=(uint16_t)(int32_t)lround(asin(nz)*(65536./TAU));
    uint16_t z=hypot(f->up.x,f->up.y)<1e-6?0:(uint16_t)(int32_t)lround(atan2(-(double)f->up.x,f->up.y)*(65536./TAU));
    x=toward_zero(x);z=toward_zero(z);
    double sx=frame_sine(m,x),cx=frame_sine(m,(uint16_t)(x+0x4000));
    double sy=frame_sine(m,y),cy=frame_sine(m,(uint16_t)(y+0x4000));
    double sz=frame_sine(m,z),cz=frame_sine(m,(uint16_t)(z+0x4000));
    SA1SurfaceFrame n={.forward={(float)(cz*cy+sz*sx*sy),(float)(sz*cy-cz*sx*sy),(float)(cx*sy)},
        .up={(float)(-sz*cx),(float)(cz*cx),(float)sx},
        .right={(float)(cz*sy-sz*sx*cy),(float)(sz*sy+cz*sx*cy),(float)(-cx*cy)},.valid=1};
    if(!finite3(n.up)||!finite3(n.forward)||!finite3(n.right))return 0;
    *f=n;return 1;
}
