#include "steam_flat.h"
#include "steam_surface.h"
#include "surface_frame.h"
#include "sonic_animation_runtime.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static const float pi=3.14159265358979323846f;
static SA1Surface floor_surface(void*c,float x,float z){
    (void)c;(void)x;(void)z;return (SA1Surface){.ground_y=0,.normal={0,1,0},.valid=true};
}
static SA1Contact floor_contact(void*c,SA1Vec3 p,SA1Vec3 probe,float range){
    (void)c;(void)probe;return (SA1Contact){.point={p.x,0,p.z},.normal={0,1,0},
        .signed_distance=p.y,.surface_id=0,.valid=p.y>=-.35f&&p.y<=range};
}
static SA1PhysicsWorld world={.query_surface=floor_surface,.query_contact=floor_contact,
    .reset_position={0,1,0},.reset_min_x=-10000,.reset_max_x=10000,.reset_min_y=-10000,.reset_min_z=-10000,.reset_max_z=10000};
static SA1PlayerState moving(float speed){
    SA1PlayerState p;sa1_player_reset(&p,&world);p.pc2004_action_state=2;
    p.vel.z=speed;p.surface_id=0;return p;
}
static void step(SA1PlayerState*p,SA1Input in){
    sa1_physics_step_steam_sonic_flat(p,&in,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,0);
}
static void publish(SA1PlayerState*p,const SA3DSSonicAnimationRuntime*r){
    p->animation_previous_request=r->requested_runtime_id;p->animation_prior_source=r->prior_source_runtime_id;
    p->animation_previous_source=r->deferred_index_valid?r->deferred_index_runtime_id:r->active_binding.runtime_id;
    p->animation_previous_request_valid=r->initialized!=0;
}
static SA3DSSonicAnimationSelection animate(SA1PlayerState*p,SA3DSSonicAnimationRuntime*r){
    SA3DSSonicAnimationObservation o={.intent=SA3DS_SONIC_ANIM_GROUNDED_BELOW_JOG};
    return sa3ds_sonic_animation_select(r,p,&o);
}
int main(void){
    SA1Input opposed={.move_z=-1},forward={.move_z=1};
    SA1PlayerState p=moving(2);
    assert(!sa1_steam_sonic_brake_dispatch(&p,&forward,pi*.75f));
    assert(sa1_steam_sonic_brake_dispatch(&p,&forward,pi*.75f+2*pi/65536.f));
    p.vel.z=nextafterf(SA1_STEAM_SONIC_PARAMETER_EVIDENCE.jog_speed,0);
    assert(!sa1_steam_sonic_brake_dispatch(&p,&opposed,0));
    p.vel.z=SA1_STEAM_SONIC_PARAMETER_EVIDENCE.jog_speed;
    assert(sa1_steam_sonic_brake_dispatch(&p,&opposed,0));
    p.pc2004_status_flags=0x2000;assert(!sa1_steam_sonic_brake_dispatch(&p,&opposed,0));
    p.pc2004_status_flags=0x4000;assert(!sa1_steam_sonic_brake_dispatch(&p,&opposed,0));
    p=moving(2);step(&p,opposed);
    assert(p.pc2004_action_state==13&&p.animation_script_request==20&&p.animation_brake_write);
    assert(p.yaw==0&&p.vel.z>0&&p.vel.z<2&&p.grounded&&!p.ball_form);
    float previous=p.vel.z;
    for(int i=0;i<3;i++){
        p.animation_previous_request=20;p.animation_previous_request_valid=true;
        step(&p,opposed);assert(p.pc2004_action_state==13&&p.yaw==0&&p.vel.z<previous);
        assert(!p.animation_brake_write);previous=p.vel.z;
    }
    /* End-to-end physics -> actual playback -> next physics update.
     * Turn-around30 is retained until NextAnim, then +128/index mismatch
     * adds jog speed once. No animation request is inferred from a timer. */
    p=moving(.7f);SA3DSSonicAnimationRuntime runtime={0};(void)animate(&p,&runtime);
    int saw20=0,saw30=0,saw_tail=0,turn_ticks=0;
    for(int i=0;i<55;i++){
        publish(&p,&runtime);
        int tail=p.animation_prior_source==30&&p.animation_previous_source!=30;
        float before=sa1_horizontal_speed(&p);
        step(&p,opposed);SA3DSSonicAnimationSelection selected=animate(&p,&runtime);
        if(selected.requested_runtime_id==20)saw20=1;
        if(selected.requested_runtime_id==30){saw30=1;turn_ticks++;}
        if(tail){assert(sa1_horizontal_speed(&p)>before+.3f);saw_tail=1;}
    }
    assert(saw20&&saw30&&saw_tail&&turn_ticks>=16);
    /* Jump wins before brake, and action input does not start charging from13. */
    p=moving(1);p.pc2004_action_state=13;p.animation_previous_request=20;p.animation_previous_request_valid=true;
    SA1Input jump=opposed;jump.jump_pressed=true;step(&p,jump);
    assert(p.pc2004_action_state==8&&!p.grounded&&p.ball_form);
    p=moving(1);p.pc2004_action_state=13;p.animation_previous_request=20;p.animation_previous_request_valid=true;
    SA1Input action=opposed;action.action_pressed=true;step(&p,action);
    assert(p.pc2004_action_state==13&&p.action_state!=SA1_ACTION_SPINDASH_CHARGE);
    /* Automatic completion exits to1 for this update even with analog held. */
    p=moving(1);p.pc2004_action_state=13;p.animation_previous_request=0;p.animation_previous_request_valid=true;
    step(&p,forward);assert(p.pc2004_action_state==1&&p.animation_brake_write&&p.animation_script_request==0);
    puts("PC2004 brake: PASS strict angle/speed gates, fixed facing, release, jump priority, turn-around playback and saved-index boost; full retail route parity separate");
    return 0;
}
