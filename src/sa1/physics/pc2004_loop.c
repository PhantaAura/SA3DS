#include "pc2004_loop.h"
#include "pc2004_task_frame.h"
#include <math.h>
#include <float.h>
int sa1_pc2004_loop_sample(const SA1PC2004Loop *p,float d,SA1Vec3 *out){
    if(!p||!out||p->count<2||!isfinite(d)||!(p->length>0))return 0;
    if(p->wrap){d=fmodf(d,p->length);if(d<0)d+=p->length;}
    else if(d<0||(double)p->length*.001<(double)d*.001)return 0;
    float start=0;
    /* 49C330 includes the final zero-length point, with a unit interval. */
    for(uint32_t i=0;i<p->count;i++){
        float length=p->points[i].length;
        if(length==0)length=1;
        float end=start+length;
        if(d<end){
            float t=(d-start)/length;
            uint32_t next=i+1<p->count?i+1:(p->wrap?0:i);
            SA1Vec3 a=p->points[i].position,b=p->points[next].position;
            *out=(SA1Vec3){a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};
            return 1;
        }
        start=end;
    }
    return 0;
}
float sa1_pc2004_loop_nearest(const SA1PC2004Loop *p,SA1Vec3 v){
    if(!p||p->count<2)return 0;
    float best=FLT_MAX,parameter=0,start=0;
    for(uint32_t i=0;i<p->count-(p->wrap?0u:1u);i++){
        SA1Vec3 a=p->points[i].position,b=p->points[(i+1)%p->count].position;
        SA1Vec3 edge=sa1_vec3_sub(b,a);
        float square=sa1_vec3_dot(edge,edge);
        float t=square>0?sa1_vec3_dot(sa1_vec3_sub(v,a),edge)/square:0;
        t=fmaxf(0,fminf(1,t));
        SA1Vec3 offset=sa1_vec3_scale(edge,t);
        SA1Vec3 delta=sa1_vec3_sub(v,sa1_vec3_add(a,offset));
        float distance=sa1_vec3_dot(delta,delta);
        if(distance<best){best=distance;parameter=start+sa1_vec3_length(offset);}
        start+=p->points[i].length;
    }
    return parameter;
}
void sa1_pc2004_loop_begin(SA1PlayerState *p,float run_speed){
    if(!p||!p->pc2004_loop_pending)return;
    p->pc2004_loop_pending=false;
    SA1Vec3 forward=p->surface_frame_active?p->surface_forward:
        (SA1Vec3){sinf(p->yaw),0,cosf(p->yaw)};
    if(p->pc2004_loop&&(p->pc2004_action_state==2||p->pc2004_action_state==5||
        sa1_vec3_dot(p->vel,forward)>run_speed))p->pc2004_status_flags|=0x2000u;
}
int sa1_pc2004_loop_steer(SA1PlayerState *p,SA1SurfaceFrame *frame,
    float *forward,float dash_speed,SA1Vec3 motion){
    if(*forward<dash_speed)*forward=dash_speed;
    p->pc2004_loop_distance+=*forward;
    SA1Vec3 target;
    if(!sa1_pc2004_loop_sample(p->pc2004_loop,p->pc2004_loop_distance,&target))goto end;
    SA1Vec3 origin=sa1_vec3_sub(p->pos,sa1_vec3_scale(frame->up,1.f));
    SA1Vec3 desired=sa1_project_on_plane(sa1_vec3_sub(target,origin),frame->up);
    motion=sa1_project_on_plane(motion,frame->up);
    if(sa1_vec3_length(desired)==0||sa1_vec3_length(motion)==0)goto end;
    desired=sa1_vec3_normalize(desired);motion=sa1_vec3_normalize(motion);
    float cosine=sa1_vec3_dot(desired,motion);
    double angle=acos(fmax(-1.,fmin(1.,cosine)));
    uint16_t signed_angle=(uint16_t)(int32_t)(angle*65536.*0.15915497620314795);
    if(sa1_vec3_dot(sa1_vec3_cross(desired,motion),frame->up)<0)signed_angle=(uint16_t)(-signed_angle);
    uint16_t old;
    if(!sa1_pc2004_task_yaw(frame,&old))goto end;
    uint16_t next=(uint16_t)(old+signed_angle);
    int delta=(int16_t)(uint16_t)(next-old);
    if(delta>0x4000||delta< -0x4000)next=(uint16_t)(-next-0x8000);
    float turn=(int16_t)(uint16_t)(next-old)*(6.283185307179586f/65536.f);
    frame->forward=sa1_vec3_normalize(sa1_vec3_sub(
        sa1_vec3_scale(frame->forward,cosf(turn)),sa1_vec3_scale(frame->right,sinf(turn))));
    frame->right=sa1_vec3_normalize(sa1_vec3_cross(frame->up,frame->forward));
    return 1;
end:
    p->pc2004_status_flags&=(uint16_t)~0x2000u;return 0;
}
