#include "checkpoint_player.h"
#include "checkpoint_task.h"
#include "emerald_coast_collision.h"
#include "../sa1/physics/movement.h"
#include "../sa1/physics/pc2004_task_frame.h"
#include <math.h>
static int finite3(SA1Vec3 v){return isfinite(v.x)&&isfinite(v.y)&&isfinite(v.z);}
static SA1SurfaceFrame frame(const SA1PlayerState*p){
    SA1Vec3 forward=p->surface_frame_active?p->surface_forward:(SA1Vec3){sinf(p->yaw),0,cosf(p->yaw)};
    return sa1_surface_frame_make(p->ground_normal,forward);
}
static int angles(const SA1SurfaceFrame*f,int32_t out[3]){
    uint16_t yaw;if(!sa1_pc2004_task_yaw(f,&yaw))return 0;
    const double to_bams=65536./6.283185307179586476925286766559;
    out[0]=(uint16_t)(int32_t)lround(asin(fmax(-1.,fmin(1.,f->up.z)))*to_bams);
    out[1]=yaw;
    out[2]=hypot(f->up.x,f->up.y)<1e-6?0:(uint16_t)(int32_t)lround(atan2(-(double)f->up.x,f->up.y)*to_bams);
    return 1;
}
int sa3ds_checkpoint_capture_player(SA3DSRestartRecord*r,const SA1PlayerState*p,int16_t act,const uint8_t time[3]){
    if(!r||!p||!time)return 0;
    SA1SurfaceFrame f=frame(p);int32_t a[3];if(!angles(&f,a))return 0;
    SA1Vec3 foot=sa1_vec3_sub(p->pos,sa1_vec3_scale(f.up,SA3DS_EC_CONTROLLER_SUPPORT));
    sa3ds_restart_record_store(r,1,act,foot,a,time);return 1;
}
static double value(double(*sine)(void*,uint16_t),void*ctx,int32_t a){return sine?sine(ctx,(uint16_t)a):sin((uint16_t)a*(6.2831853071795864769/65536.));}
int sa3ds_checkpoint_restore_player(const SA3DSRestartRecord*r,SA1PlayerState*p,const SA1PhysicsWorld*w,double(*sine)(void*,uint16_t),void*ctx){
    if(!r||!p||!w||!r->valid||!finite3(r->task_position))return 0;
    const int32_t*a=r->task_angles;
    double sx=value(sine,ctx,a[0]),cx=value(sine,ctx,(uint16_t)((uint32_t)a[0]+16384u));
    double sy=value(sine,ctx,a[1]),cy=value(sine,ctx,(uint16_t)((uint32_t)a[1]+16384u));
    double sz=value(sine,ctx,a[2]),cz=value(sine,ctx,(uint16_t)((uint32_t)a[2]+16384u));
    SA1SurfaceFrame f={.forward={(float)(cz*cy+sz*sx*sy),(float)(sz*cy-cz*sx*sy),(float)(cx*sy)},
        .up={(float)(-sz*cx),(float)(cz*cx),(float)sx},
        .right={(float)(cz*sy-sz*sx*cy),(float)(sz*sy+cz*sx*cy),(float)(-cx*cy)},.valid=1};
    if(!finite3(f.up)||!finite3(f.forward)||!finite3(f.right))return 0;
    sa1_player_reset(p,w);p->ground_normal=f.up;p->surface_forward=f.forward;p->surface_right=f.right;p->surface_frame_active=true;
    p->pos=sa1_vec3_add(r->task_position,sa1_vec3_scale(f.up,SA3DS_EC_CONTROLLER_SUPPORT));
    p->peak_y=p->pos.y;p->yaw=atan2f(f.forward.x,f.forward.z);p->surface_id=-1;
    return 1;
}
int sa3ds_checkpoint_player_spin(const SA1PlayerState*p,uint16_t yaw,int16_t*out){
    if(!p||!out)return 0;
    SA1SurfaceFrame f=frame(p);uint16_t player_yaw;if(!sa1_pc2004_task_yaw(&f,&player_yaw))return 0;
    float forward=sa1_vec3_dot(p->vel,f.forward),normal=sa1_vec3_dot(p->vel,f.up),side=-sa1_vec3_dot(p->vel,f.right);
    double speed=(float)sqrt((float)((double)side*side+(double)normal*normal+(double)forward*forward));
    if(sa1_pc2004_homing_angle_distance((uint16_t)(16384-player_yaw),yaw)>16384)speed=-speed;
    return sa3ds_checkpoint_spin(speed,out);
}
