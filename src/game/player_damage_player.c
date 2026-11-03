#include "player_damage_player.h"
#include <math.h>
#include "../sa1/physics/steam_input.h"
#include "../sa1/physics/surface_frame.h"
static void request(SA1PlayerState*p,uint16_t id){p->animation_script_serial++;p->animation_script_request=id;p->animation_script_valid=true;}
int sa3ds_player_damage_prepare(SA1PlayerState*p,const SA1Input*input,uint16_t ccl,uint16_t animation,int16_t*rings,
    const SA3DSPlayerDamageWorld*w,const SA1SteamPlayerParameters*parameters,SA3DSPlayerDamageResult*out){
    if(!p||!input||!rings||!w||!parameters||!out)return 0;
    SA1Vec3 forward=p->surface_frame_active?p->surface_forward:(SA1Vec3){sinf(p->yaw),0,cosf(p->yaw)};
    SA1SurfaceFrame frame=sa1_surface_frame_make(p->ground_normal,forward);if(!frame.valid)return 0;
    SA1Vec3 foot=sa1_vec3_sub(p->pos,frame.up); /*inherited1-unit support coordinate*/
    SA3DSPlayerDamageState s={.action=p->pc2004_action_state,.player_index=0,
        .status=(uint16_t)((p->pc2004_status_flags&~0x13fu)|(p->ball_form?0x100:0)|(p->grounded?1:0)|(ccl&0x3c)),
        .timer=p->pc2004_hurt_timer,.flags4=p->pc2004_player_flags4,.flags6=p->pc2004_player_flags6,
        .flags5c=p->pc2004_player_flags5c,.animation=animation,.rings=*rings,
        .speed={sa1_vec3_dot(p->vel,frame.forward),sa1_vec3_dot(p->vel,frame.up),-sa1_vec3_dot(p->vel,frame.right)},
        .position=foot,.collision_up=frame.up};
    SA1PC2004LocalVector previous_speed=s.speed;
    if(!sa3ds_player_damage_tick(&s,w,parameters,out))return 0;
    int write=out->accepted||s.animation!=animation;
    SA3DSPlayerHurtResult hurt={0};
    if(s.action==16&&!(s.status&0x1000)){
        if(!sa3ds_player_hurt_select(&s,sa1_sadx_filter_logical_analog(input->move_x,input->move_z).active,input->jump_pressed,&hurt))return 3;
        write|=hurt.write_animation;
    }
    p->pc2004_hurt_timer=s.timer;p->pc2004_status_flags=(uint16_t)(s.status&~0x103u);
    p->pc2004_player_flags4=s.flags4;p->pc2004_player_flags6=s.flags6;p->pc2004_player_flags5c=s.flags5c;
    *rings=s.rings;
    if(out->accepted||hurt.resume){p->pc2004_action_state=s.action;p->pc2004_action_state_valid=true;p->ball_form=(s.status&0x100)!=0;}
    if(write)request(p,s.animation);
    if(out->accepted){p->animation_airborne_request_valid=false;p->spindash_charge=0;p->spindash_ticks=0;}
    if(s.speed.forward!=previous_speed.forward||s.speed.normal!=previous_speed.normal||s.speed.side!=previous_speed.side)
        p->vel=sa1_vec3_add(sa1_vec3_add(sa1_vec3_scale(frame.forward,s.speed.forward),sa1_vec3_scale(frame.up,s.speed.normal)),sa1_vec3_scale(frame.right,-s.speed.side));
    if(s.position.x!=foot.x||s.position.y!=foot.y||s.position.z!=foot.z)p->pos=sa1_vec3_add(s.position,frame.up);
    if(out->death_requested)return 2;
    if(out->water_effect||out->release_held||out->release_light_dash)return 3;
    if(hurt.begin_jump){p->pc2004_action_state=2;p->action_state=SA1_ACTION_GROUNDED;}
    else if(p->pc2004_action_state_valid&&p->pc2004_action_state==16)p->action_state=SA1_ACTION_HURT;
    else if(hurt.resume){
        p->action_state=p->grounded?SA1_ACTION_GROUNDED:SA1_ACTION_AIRBORNE;
        if(s.action==12){p->animation_airborne_request=18;p->animation_airborne_request_valid=true;}
    }
    return 1;
}
