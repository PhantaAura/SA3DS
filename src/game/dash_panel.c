#include "dash_panel.h"
#include <math.h>
#include <string.h>
int sa3ds_dash_panel_parameters(const SA3DSMovementPlacement *p,SA3DSDashPanelParameters *out){
    if(!out)return 0;
    *out=(SA3DSDashPanelParameters){0};
    if(!p||(p->type&4095)!=3||!isfinite(p->parameters[0])||!isfinite(p->parameters[1]))return 0;
    /* Init replaces nonpositive Scale.x/Scale.y with14/60. The task takes
     * ceil(duration) at644B20 before the integer conversion at644340. */
    float speed=p->parameters[0]>0?p->parameters[0]:14.f;
    double duration=ceil(p->parameters[1]>0?(double)p->parameters[1]:60.);
    if(duration>2147483647.)return 0;
    int32_t lockout=(int32_t)duration;
    uint16_t low=(uint16_t)(uint32_t)lockout;int16_t timer;
    memcpy(&timer,&low,2);
    *out=(SA3DSDashPanelParameters){speed,lockout,timer};return 1;
}
int sa3ds_dash_panel_tick(SA3DSDashPanel *a,const SA3DSMovementPlacement *p,
    const SA3DSDashPanelInput *in,SA3DSDashPanelResult *out){
    if(!out)return 0;
    *out=(SA3DSDashPanelResult){0};
    SA3DSDashPanelParameters params;
    if(!a||!in||a->initialized>1||a->cooldown<0||
       in->first_hit_player < -1||in->first_hit_player>1||
       !sa3ds_dash_panel_parameters(p,&params))return 0;
    if(in->skip_update)return 1;
    SA3DSDashPanel next=*a;SA3DSDashPanelResult result={0};
    if(!next.initialized){next.initialized=1;next.last_player=13;}
    else{
        result.draw=result.enqueue_collision=1;
        if(next.cooldown>0)next.cooldown--;
        if(in->first_hit_player>=0&&in->player_ground_status_bit0&&
           (!next.cooldown||next.last_player!=(uint8_t)in->first_hit_player)){
            result.boost=1;result.player=(uint8_t)in->first_hit_player;result.parameters=params;
            next.last_player=result.player;next.cooldown=params.lockout;
        }
    }
    *a=next;*out=result;return 1;
}
static int direct_speed(SA1PlayerState *p,const SA3DSMovementPlacement *placement,
    const SA3DSSpringTrig *trig,float speed,float lift,int16_t timer,float support){
    if(!p||!placement||!isfinite(speed)||!isfinite(support)||support<0)return 0;
    SA3DSMovementPlacement orientation=*placement;orientation.type=2;
    SA1SurfaceFrame frame;
    if(!sa3ds_springb_physical_frame(&orientation,trig,&frame))return 0;
    SA1Vec3 old_up=p->surface_frame_active?p->ground_normal:(SA1Vec3){0,1,0};
    SA1Vec3 task=sa1_vec3_sub(p->pos,sa1_vec3_scale(old_up,support));
    SA1Vec3 velocity=sa1_vec3_add(sa1_vec3_scale(frame.forward,speed),sa1_vec3_scale(frame.up,lift));
    if(!isfinite(velocity.x)||!isfinite(velocity.y)||!isfinite(velocity.z))return 0;
    p->pos=sa1_vec3_add(task,sa1_vec3_scale(frame.up,support));
    p->vel=velocity;p->ground_normal=frame.up;p->surface_forward=frame.forward;
    p->surface_right=frame.right;p->surface_frame_active=true;
    if(frame.forward.x*frame.forward.x+frame.forward.z*frame.forward.z>1e-8f)
        p->yaw=atan2f(frame.forward.x,frame.forward.z);
    p->pc2004_control_timer=timer;return 1;
}

int sa3ds_dash_panel_apply(SA1PlayerState *p,const SA3DSMovementPlacement *placement,
    const SA3DSSpringTrig *trig,const SA3DSDashPanelParameters *params,float support){
    if(!placement||!params||(placement->type&4095)!=3||params->speed<=0)return 0;
    return direct_speed(p,placement,trig,params->speed,0.f,params->control_timer,support);
}

int sa3ds_jump_pad_apply(SA1PlayerState *p,const SA3DSMovementPlacement *placement,
    const SA3DSSpringTrig *trig,float support){
    if(!placement||!trig||(placement->type&4095)!=40)return 0;
    double duration=ceil(placement->parameters[1]);
    if(!isfinite(duration)||duration<-2147483648.||duration>2147483647.)return 0;
    uint16_t low=(uint16_t)(int32_t)duration;int16_t timer;
    memcpy(&timer,&low,sizeof(timer));
    SA3DSMovementPlacement rotated=*placement;
    uint16_t yaw=(uint16_t)((uint16_t)placement->angles[1]+0x8000u);
    memcpy(&rotated.angles[1],&yaw,sizeof(yaw));
    SA3DSSpringTrig launch_trig=*trig;
    launch_trig.sine[1]=-trig->sine[1];
    launch_trig.cosine[1]=-trig->cosine[1];
    return direct_speed(p,&rotated,&launch_trig,placement->parameters[0],4.f,timer,support);
}

int sa3ds_dash_panel_primary_overlap(const SA3DSMovementPlacement *p,
    const SA3DSSpringTrig *trig,SA1Vec3 center,float radius){
    if(!p||(p->type&4095)!=3||!isfinite(radius)||radius<0||
       !isfinite(center.x)||!isfinite(center.y)||!isfinite(center.z)||
       !isfinite(p->position.x)||!isfinite(p->position.y)||!isfinite(p->position.z))return -1;
    SA3DSMovementPlacement orientation=*p;orientation.type=2;SA1SurfaceFrame frame;
    if(!sa3ds_springb_physical_frame(&orientation,trig,&frame))return -1;
    /* 419470 transforms local Up by SETZ/SETX/SETY then record X4000:
     * that is the same column as the physical forward from441490. */
    SA1Vec3 extent=sa1_vec3_scale(frame.forward,4.44f);
    SA1Vec3 a=sa1_vec3_add(p->position,extent),b=sa1_vec3_sub(p->position,extent);
    SA1Vec3 segment=sa1_vec3_sub(b,a),offset=sa1_vec3_sub(center,a);
    double length2=(double)segment.x*segment.x+(double)segment.y*segment.y+(double)segment.z*segment.z;
    if(!(length2>0))return -1;
    double t=((double)offset.x*segment.x+(double)offset.y*segment.y+(double)offset.z*segment.z)/length2;
    SA1Vec3 foot={(float)((double)a.x+t*segment.x),(float)((double)a.y+t*segment.y),
        (float)((double)a.z+t*segment.z)};
    SA1Vec3 delta=sa1_vec3_sub(center,foot);
    float distance=(float)sqrt((double)delta.x*delta.x+(double)delta.y*delta.y+(double)delta.z*delta.z);
    float reach=radius+10.f;
    if(!(distance<reach))return 0;
    if(t>0&&t<1)return 1;
    delta=sa1_vec3_sub(center,t<=0?a:b);
    float endpoint2=(float)((double)delta.x*delta.x+(double)delta.y*delta.y+(double)delta.z*delta.z);
    return endpoint2<reach*reach;
}
