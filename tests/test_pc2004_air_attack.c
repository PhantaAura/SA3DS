#include "pc2004_air_attack.h"
#include "movement.h"
#include "steam_flat.h"
#include "surface_frame.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int closef(float a,float b){return fabsf(a-b)<.000003f;}
static SA1Surface flat_floor(void*c,float x,float z){(void)c;(void)x;(void)z;return (SA1Surface){.ground_y=0,.normal={0,1,0},.valid=true};}
static SA1Contact flat_contact(void*c,SA1Vec3 p,SA1Vec3 probe,float range){
    (void)c;(void)probe;return (SA1Contact){.point={p.x,0,p.z},.normal={0,1,0},.signed_distance=p.y,.surface_id=0,.valid=p.y>=-.35f&&p.y<=range};
}
static SA1Surface none(void*c,float x,float z){(void)c;(void)x;(void)z;return (SA1Surface){0};}
int main(void){
    SA1PC2004AirAttack initial={8,0x500,15,0,0};
    SA1PC2004AirAttackSelection s=sa1_pc2004_air_attack_select_no_target(initial,1,0,-1,0);
    assert(s.entered&&s.write_forward&&s.forward==5&&s.next.action==14&&s.next.timer==1&&s.next.age==0);
    assert(s.next.animation==15&&s.next.status==0x500);
    SA1PC2004LocalVector speed={5,2,3};
    assert(sa1_pc2004_air_attack_decay(&s.next,&speed)&&s.next.timer==2&&speed.forward==4.5f&&speed.normal==2&&speed.side==3);
    for(int timer=2;timer<6;timer++){
        s=sa1_pc2004_air_attack_select_no_target(s.next,0,0,-1,0);
        assert(s.next.action==14);assert(sa1_pc2004_air_attack_decay(&s.next,&speed));
    }
    assert(s.next.timer==6&&closef(speed.forward,2.9524498f));
    s=sa1_pc2004_air_attack_select_no_target(s.next,0,0,-1,0);
    assert(s.next.action==12&&s.next.animation==18&&s.next.timer==0&&!(s.next.status&0x500));
    s=sa1_pc2004_air_attack_select_no_target(initial,1,1,-1,0);
    assert(!s.entered&&s.next.action==12&&s.next.animation==18);
    initial.status|=0x4000;s=sa1_pc2004_air_attack_select_no_target(initial,1,0,-1,0);
    assert(!s.entered&&s.next.action==8);initial.status=0x8500;
    s=sa1_pc2004_air_attack_select_no_target(initial,1,0,-1,0);assert(s.forward==5);
    s=sa1_pc2004_air_attack_select_no_target(initial,1,0,-1,1);assert(s.forward==10);
    for(int category=0;category<=4;category++){
        SA1PC2004AirAttack a={14,0x500,14,6,10};s=sa1_pc2004_air_attack_select_no_target(a,0,1,category,0);
        if(category==0||category==2||category==3)assert(s.next.action==8&&s.next.timer==-1&&s.next.animation==14&&s.next.status==0x500);
        else assert(s.next.action==12&&s.next.timer==0);
    }
    SA1PC2004AirAttack a={14,0x500,14,0,359};s=sa1_pc2004_air_attack_select_no_target(a,0,0,-1,0);
    assert(s.next.action==14&&s.next.age==360);s=sa1_pc2004_air_attack_select_no_target(s.next,0,0,-1,0);
    assert(s.next.action==8&&s.next.timer==-1&&s.skip_jump_hold);
    a.action=12;s=sa1_pc2004_air_attack_select_no_target(a,1,0,-1,0);assert(s.next.action==12&&!s.entered);
    /* Actual public controller: second jump enters movement the same tick,
     * with original ground acceleration, retained yaw and no held-jump boost. */
    SA1PhysicsWorld world={.query_surface=none,.reset_position={0,100,0},
        .reset_min_x=-10000,.reset_max_x=10000,.reset_min_y=-10000,.reset_min_z=-10000,.reset_max_z=10000};
    SA1PlayerState p;sa1_player_reset(&p,&world);p.grounded=false;p.action_state=SA1_ACTION_AIRBORNE;
    p.pc2004_action_state=8;p.ball_form=true;p.yaw=1.5707963267948966f;
    p.vel=(SA1Vec3){1,2,0};p.animation_airborne_request_valid=true;p.animation_airborne_request=15;
    SA1Input input={.jump_pressed=true,.jump_held=true};
    sa1_physics_step_steam_sonic_flat(&p,&input,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,0);
    assert(p.pc2004_action_state==14&&p.pc2004_air_attack_timer==2&&p.pc2004_air_attack_age==0);
    assert(p.ball_form&&p.animation_airborne_request==15&&p.jump_ticks==0);
    assert(p.pc2004_ccl_state_valid&&p.pc2004_ccl_action==14&&p.pc2004_ccl_ball);
    assert(p.surface_frame_active&&closef(p.yaw,1.5707963267948966f));
    /* Positive speed/negative slowdown bypasses the positive-input friction
     * cap:4.5 + 4.5*(-.008) + (-.06) =4.404. */
    assert(closef(p.vel.x,4.404f));assert(closef(p.vel.y,1.9f));
    input.jump_pressed=false;
    for(int tick=0;tick<4;tick++){
        sa1_physics_step_steam_sonic_flat(&p,&input,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,0);
        assert(p.pc2004_action_state==14&&p.jump_ticks==0);
    }
    assert(p.pc2004_air_attack_timer==6);
    sa1_physics_step_steam_sonic_flat(&p,&input,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,0);
    assert(p.pc2004_action_state==12&&!p.ball_form&&p.animation_airborne_request==18&&p.jump_ticks==0);
    assert(p.pc2004_ccl_state_valid&&p.pc2004_ccl_action==12&&!p.pc2004_ccl_ball);
    /* Landing must retain the selected attack strength even though the
     * completed player state is now idle/non-ball. Exercise both dispatches. */
    world.query_surface=flat_floor;
    for(unsigned general=0;general<2;general++){
        world.query_contact=general?flat_contact:NULL;
        sa1_player_reset(&p,&world);p.pos=(SA1Vec3){0,1.2f,0};p.vel=(SA1Vec3){0,-.5f,0};
        p.grounded=false;p.action_state=SA1_ACTION_AIRBORNE;p.pc2004_action_state=8;p.ball_form=true;
        p.surface_frame_active=general!=0;
        sa1_physics_step_steam_sonic_flat(&p,&(SA1Input){0},0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,0);
        assert(p.grounded&&!p.ball_form&&p.pc2004_action_state==1);
        assert(p.pc2004_ccl_state_valid&&p.pc2004_ccl_action==8&&p.pc2004_ccl_ball);
        sa1_physics_step_steam_sonic_flat(&p,&(SA1Input){0},0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,0);
        assert(p.pc2004_ccl_state_valid&&p.pc2004_ccl_action==1&&!p.pc2004_ccl_ball);
        sa1_physics_step_steam_sonic_flat(&p,&(SA1Input){.jump_pressed=true},0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,0);
        assert(p.pc2004_ccl_state_valid&&p.pc2004_ccl_action==8&&p.pc2004_ccl_ball);
    }
    puts("PC2004 air attack: PASS source empty-target entry/decay/timeout/cancel/CCL priority/age/status separation and public same-tick dispatch and flat/general pre-contact attack snapshots through landing; homing pursuit not tested");
}
