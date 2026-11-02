#include "spring_player.h"
#include <math.h>
int sa3ds_spring_player_apply(SA1PlayerState *p,
    const SA3DSMovementPlacement *placement,const SA3DSSpringTrig *trig,
    const SA3DSSpringLaunch *launch,float support,SA3DSSpringPendingCommand *pending){
    if(!p||!placement||!launch||!pending||!isfinite(support)||support<0)return 0;
    SA1SurfaceFrame frame=p->surface_frame_active?
        (SA1SurfaceFrame){p->ground_normal,p->surface_forward,p->surface_right,1}:
        sa1_surface_frame_make(p->ground_normal,(SA1Vec3){sinf(p->yaw),0,cosf(p->yaw)});
    if((placement->type&4095)==2){
        if(!sa3ds_springb_physical_frame(placement,trig,&frame))return 0;
    }else if((placement->type&4095)!=1)return 0;
    if(sa1_vec3_length(frame.up)<.5f||sa1_vec3_length(frame.forward)<.5f)return 0;
    /* Source command helpers retain low ground bits until position resolution.
     * This port currently models ordinary low-ground and ball/charge bits. */
    uint16_t status=0x1000u|(p->grounded?1u:0u)|(p->ball_form?0x100u:0u)|
        (p->action_state==SA1_ACTION_SPINDASH_CHARGE?0x400u:0u);
    p->pos=sa1_vec3_add(launch->position,sa1_vec3_scale(frame.up,support));
    p->vel=launch->world_velocity;
    p->ground_normal=frame.up;p->surface_forward=frame.forward;p->surface_right=frame.right;
    p->surface_frame_active=true;
    if(frame.forward.x*frame.forward.x+frame.forward.z*frame.forward.z>1e-8f)
        p->yaw=atan2f(frame.forward.x,frame.forward.z);
    *pending=(SA3DSSpringPendingCommand){launch->command,status,1};return 1;
}
int sa3ds_spring_player_begin(SA1PlayerState *p,
    SA3DSSpringPendingCommand *pending,SA1Input *input){
    if(!p||!pending||!input)return 0;
    if(pending->valid){
        SA3DSSpringCommandResult command;
        if(!sa3ds_spring_command_consume(&pending->command,pending->status,
            p->pc2004_control_timer,&command))return 0;
        p->pc2004_action_state=command.sonic_action;p->pc2004_action_state_valid=true;
        p->action_state=SA1_ACTION_AIRBORNE;
        p->ball_form=(command.task_status&0x100u)!=0;
        p->pc2004_control_timer=command.control_timer;
        p->animation_airborne_request=command.animation_request;
        p->animation_airborne_request_valid=true;
        pending->valid=0;
    }
    if(p->pc2004_action_state_valid&&
       (p->pc2004_action_state==9||p->pc2004_action_state==10)){
        SA3DSSpringActionResult action;
        uint16_t animation=p->animation_airborne_request_valid?p->animation_airborne_request:16;
        if(!sa3ds_spring_action_select(p->pc2004_action_state,animation,p->vel.y,
            sa1_vec3_dot(p->vel,p->ground_normal),&SA1_STEAM_SONIC_PARAMETER_EVIDENCE,&action))return 0;
        p->pc2004_action_state=action.sonic_action;
        p->animation_airborne_request=action.animation_request;
        p->animation_airborne_request_valid=true;
    }
    *input=sa3ds_spring_filter_analog(*input,p->pc2004_control_timer);return 1;
}
void sa3ds_spring_player_tail(SA1PlayerState *p){
    if(p)p->pc2004_control_timer=sa3ds_spring_control_timer_tail(p->pc2004_control_timer);
}
