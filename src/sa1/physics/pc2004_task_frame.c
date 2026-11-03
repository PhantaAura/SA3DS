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
