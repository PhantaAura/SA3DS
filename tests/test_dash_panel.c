#include "dash_panel.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void){
    SA3DSMovementPlacement p={.type=3};SA3DSDashPanelParameters params;
    assert(sa3ds_dash_panel_parameters(&p,&params));
    assert(params.speed==14&&params.lockout==60&&params.control_timer==60);
    p.parameters[0]=-2;p.parameters[1]=-2;
    assert(sa3ds_dash_panel_parameters(&p,&params)&&params.speed==14&&params.lockout==60);
    p.parameters[0]=12.5f;p.parameters[1]=2.01f;
    assert(sa3ds_dash_panel_parameters(&p,&params)&&params.speed==12.5f&&params.lockout==3);
    p.parameters[1]=32768;
    assert(sa3ds_dash_panel_parameters(&p,&params)&&params.lockout==32768&&params.control_timer==-32768);
    p.parameters[1]=65536;
    assert(sa3ds_dash_panel_parameters(&p,&params)&&params.lockout==65536&&params.control_timer==0);
    p.parameters[1]=3;
    SA3DSDashPanel a={0};SA3DSDashPanelResult r;
    SA3DSDashPanelInput in={.first_hit_player=0,.player_ground_status_bit0=1};
    assert(sa3ds_dash_panel_tick(&a,&p,&in,&r)&&!r.boost&&!r.enqueue_collision&&!r.draw&&a.last_player==13);
    in.player_ground_status_bit0=0;
    assert(sa3ds_dash_panel_tick(&a,&p,&in,&r)&&r.enqueue_collision&&r.draw&&!r.boost);
    in.player_ground_status_bit0=1;
    assert(sa3ds_dash_panel_tick(&a,&p,&in,&r)&&r.boost&&a.cooldown==3);
    assert(sa3ds_dash_panel_tick(&a,&p,&in,&r)&&!r.boost&&a.cooldown==2);
    in.skip_update=1;SA3DSDashPanel frozen=a;
    assert(sa3ds_dash_panel_tick(&a,&p,&in,&r)&&memcmp(&a,&frozen,sizeof(a))==0&&!r.draw);
    in.skip_update=0;
    assert(sa3ds_dash_panel_tick(&a,&p,&in,&r)&&!r.boost&&a.cooldown==1);
    assert(sa3ds_dash_panel_tick(&a,&p,&in,&r)&&r.boost&&a.cooldown==3);
    in.first_hit_player=1;
    assert(sa3ds_dash_panel_tick(&a,&p,&in,&r)&&r.boost&&r.player==1&&a.cooldown==3);
    SA3DSSpringTrig trig={{0,0,0},{1,1,1}};
    SA1PlayerState player={.pos={10,21,30},.vel={3,4,5},.grounded=true,
        .ground_normal={0,1,0},.action_state=SA1_ACTION_ROLL,.ball_form=true,
        .animation_airborne_request_valid=true,.animation_airborne_request=15};
    SA1PlayerState before=player;
    assert(sa3ds_dash_panel_parameters(&p,&params));
    assert(sa3ds_dash_panel_apply(&player,&p,&trig,&params,1));
    assert(player.pos.x==10&&player.pos.y==21&&player.pos.z==30);
    assert(player.vel.x==0&&player.vel.y==0&&player.vel.z==12.5f);
    assert(player.grounded&&player.action_state==before.action_state&&player.ball_form);
    assert(player.animation_airborne_request_valid&&player.animation_airborne_request==15);
    assert(player.pc2004_control_timer==3&&player.surface_frame_active);
    p.angles[1]=0x4000;trig.sine[1]=1;trig.cosine[1]=0;
    assert(sa3ds_dash_panel_apply(&player,&p,&trig,&params,1));
    assert(player.vel.x==12.5f&&player.vel.y==0&&player.vel.z==0);
    /* Full task frame changes the port's support anchor, not source task XYZ. */
    p.angles[0]=0x4000;trig.sine[0]=1;trig.cosine[0]=0;
    assert(sa3ds_dash_panel_apply(&player,&p,&trig,&params,1));
    assert(player.pos.x==10&&player.pos.y==20&&player.pos.z==31);
    assert(player.vel.x==12.5f&&player.vel.y==0&&player.vel.z==0);
    p.angles[0]=p.angles[1]=p.angles[2]=0;
    trig=(SA3DSSpringTrig){{0,0,0},{1,1,1}};
    assert(sa3ds_dash_panel_primary_overlap(&p,&trig,(SA1Vec3){13.999f,0,0},4)==1);
    assert(sa3ds_dash_panel_primary_overlap(&p,&trig,(SA1Vec3){14,0,0},4)==0);
    assert(sa3ds_dash_panel_primary_overlap(&p,&trig,(SA1Vec3){0,0,18.43f},4)==1);
    assert(sa3ds_dash_panel_primary_overlap(&p,&trig,(SA1Vec3){0,0,18.45f},4)==0);
    assert(sa3ds_dash_panel_primary_overlap(&p,&trig,(SA1Vec3){0,14,0},4)==0);
    p.angles[1]=0x4000;trig.sine[1]=1;trig.cosine[1]=0;
    assert(sa3ds_dash_panel_primary_overlap(&p,&trig,(SA1Vec3){18.43f,0,0},4)==1);
    assert(sa3ds_dash_panel_primary_overlap(&p,&trig,(SA1Vec3){0,0,14.01f},4)==0);
    assert(sa3ds_dash_panel_primary_overlap(&p,&trig,(SA1Vec3){0,0,0},-1)==-1);
    p.type=40;p.angles[1]=0;p.parameters[0]=11.f;p.parameters[1]=59.2f;
    trig=(SA3DSSpringTrig){{0,0,0},{1,1,1}};
    player=before;
    assert(sa3ds_jump_pad_apply(&player,&p,&trig,1));
    assert(player.vel.x==0&&player.vel.y==4&&player.vel.z==-11.f);
    assert(player.pos.x==10&&player.pos.y==21&&player.pos.z==30);
    assert(player.pc2004_control_timer==60&&player.grounded&&player.action_state==before.action_state);
    puts("Dash panel: PASS source init/defaults/ceil/low16, prior-hit grounded gate, per-player lockout, expiry order, direct velocity/frame without spring command");
    return 0;
}
