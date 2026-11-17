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

static uint16_t approach_angle(uint16_t from,uint16_t to,int step){
    int delta=(int16_t)(uint16_t)(to-from);
    if(delta>step)return (uint16_t)(from+step);
    if(delta< -step)return (uint16_t)(from-step);
    return to;
}

static void support_angles(SA1Vec3 up,uint16_t *x,uint16_t *z){
    double nz=fmax(-1.,fmin(1.,up.z));
    *x=(uint16_t)(int32_t)lround(asin(nz)*(65536./TAU));
    *z=hypot(up.x,up.y)<1e-6?0:(uint16_t)(int32_t)lround(atan2(-(double)up.x,up.y)*(65536./TAU));
}

static int wrapped_distance(uint16_t from,uint16_t to){
    int delta=(int16_t)(uint16_t)(to-from);
    return delta<0?-delta:delta;
}

int sa1_pc2004_task_ordinary_support_target(const SA1SurfaceFrame *frame,SA1Vec3 normal,
    uint16_t *x,uint16_t *z){
    if(!frame||!frame->valid||!finite3(frame->up)||!finite3(normal)||!x||!z)return 0;
    const double bams=65536.0*0.15915497620314795;
    uint16_t pitch=(uint16_t)(int32_t)(asin(fmax(-1.,fmin(1.,normal.z)))*bams);
    uint16_t roll=(uint16_t)(int32_t)(-asin(fmax(-1.,fmin(1.,normal.x)))*bams);
    if(normal.y<0.0f){
        uint16_t old_x,old_z;
        support_angles(frame->up,&old_x,&old_z);
        uint16_t opposite_x=(uint16_t)(0x8000u-pitch),opposite_z=(uint16_t)(0x8000u-roll);
        /* 44A106: strict X comparison. Equal distances choose reflected Z. */
        if(wrapped_distance(old_x,opposite_x)<wrapped_distance(old_z,opposite_z))pitch=opposite_x;
        else roll=opposite_z;
    }
    *x=pitch;*z=roll;return 1;
}

static double frame_sine(const SA1PC2004HomingMath*m,uint16_t a){return m&&m->sine?m->sine(m->context,a):sin(a*(TAU/65536.));}
int sa1_pc2004_task_frame_from_angles(SA1SurfaceFrame *f,uint16_t x,
    uint16_t y,uint16_t z,const SA1PC2004HomingMath *m){
    if(!f)return 0;
    double sx=frame_sine(m,x),cx=frame_sine(m,(uint16_t)(x+0x4000));
    double sy=frame_sine(m,y),cy=frame_sine(m,(uint16_t)(y+0x4000));
    double sz=frame_sine(m,z),cz=frame_sine(m,(uint16_t)(z+0x4000));
    SA1SurfaceFrame n={.forward={(float)(cz*cy+sz*sx*sy),(float)(sz*cy-cz*sx*sy),(float)(cx*sy)},
        .up={(float)(-sz*cx),(float)(cz*cx),(float)sx},
        .right={(float)(cz*sy-sz*sx*cy),(float)(sz*sy+cz*sx*cy),(float)(-cx*cy)},.valid=1};
    if(!finite3(n.up)||!finite3(n.forward)||!finite3(n.right))return 0;
    *f=n;return 1;
}

int sa1_pc2004_task_relax_up(SA1SurfaceFrame*f,const SA1PC2004HomingMath*m){
    uint16_t y;if(!sa1_pc2004_task_yaw(f,&y))return 0;
    uint16_t x,z;
    support_angles(f->up,&x,&z);
    return sa1_pc2004_task_frame_from_angles(f,approach_angle(x,0,0x800),y,
        approach_angle(z,0,0x800),m);
}

int sa1_pc2004_task_approach_support(SA1SurfaceFrame *frame, SA1Vec3 normal,
    uint16_t target_x, uint16_t target_z, float forward_speed,
    const SA1SteamPlayerParameters *p, const SA1PC2004HomingMath *math){
    uint16_t yaw;
    if(!p||!finite3(normal)||!sa1_pc2004_task_yaw(frame,&yaw))return 0;

    /* For world-down gravity the source smooths faces with a lower or
     * equal Up.Y. Other faces set the target angles immediately. */
    if(normal.y<=frame->up.y){
        float local_forward=sa1_vec3_dot(frame->forward,normal);
        float local_side=sa1_vec3_dot(frame->right,normal);
        float local_up=sa1_vec3_dot(frame->up,normal);
        float horizontal=sqrtf(local_forward*local_forward+local_side*local_side);
        int step=(int)(atan2((double)horizontal,local_up)*(65536.0*0.15915497620314795));
        if(forward_speed<p->dash_speed){
            if(forward_speed<p->jog_speed)step=0x400;
            else {
                int cap=forward_speed<p->run_speed?0x1000:forward_speed<p->rush_speed?0x1500:0x2000;
                if(step>cap)step=cap;
            }
        }
        uint16_t old_x,old_z;
        support_angles(frame->up,&old_x,&old_z);
        target_x=approach_angle(old_x,target_x,step);
        target_z=approach_angle(old_z,target_z,step);
    }
    return sa1_pc2004_task_frame_from_angles(frame,target_x,yaw,target_z,math);
}
