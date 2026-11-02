#include "spring_player.h"
#include "stage.h"
#include "steam_flat.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void near(float a,float b){if(fabsf(a-b)>3e-6f){fprintf(stderr,"spring expected %.9g got %.9g\n",b,a);assert(0);}}
static void step(SA1PlayerState *p,SA3DSSpringPendingCommand *command,SA1Input input){
    assert(sa3ds_spring_player_begin(p,command,&input));
    sa1_physics_step_steam_sonic_flat(p,&input,0,sa3ds_movement_lab_world(),&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
    sa3ds_spring_player_tail(p);
}
int main(void){
    SA1PlayerState p;sa1_player_reset(&p,sa3ds_movement_lab_world());
    SA3DSMovementPlacement placement={.type=2,.parameters={12,3,0}};
    SA3DSSpringTrig trig={{0,0,0},{1,1,1}};
    SA3DSSpringLaunch launch;
    assert(sa3ds_spring_launch_prepare(&placement,&trig,(SA1Vec3){0},1,2,&launch));
    p.ball_form=true;p.action_state=SA1_ACTION_ROLL;p.pc2004_action_state=5;
    SA3DSSpringPendingCommand pending={0};
    assert(sa3ds_spring_player_apply(&p,&placement,&trig,&launch,1,&pending));
    assert(p.grounded&&p.pc2004_action_state==5&&pending.valid); /* object phase */
    near(p.pos.y,9);near(p.vel.y,8);
    SA1Input input={.move_x=1,.jump_held=true};
    assert(sa3ds_spring_player_begin(&p,&pending,&input));
    assert(p.grounded&&p.pc2004_action_state==10&&!p.ball_form&&!pending.valid);
    assert(p.animation_airborne_request==16&&p.pc2004_control_timer==12&&input.move_x==0);
    sa1_physics_step_steam_sonic_flat(&p,&input,0,sa3ds_movement_lab_world(),&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
    sa3ds_spring_player_tail(&p);
    assert(!p.grounded&&p.pc2004_action_state==10&&p.surface_frame_active);
    near(p.vel.y,7.84f);near(p.pos.y,16.84f);assert(p.pc2004_control_timer==11&&p.jump_ticks==0);
    /* Equality3 uses4432E0's -.008 forward drag; immediately below uses
     *44B9C0's -.028. Neither branch adds action8's held-jump contribution. */
    for(unsigned below=0;below<2;below++){
        p.pos=(SA1Vec3){0,100,0};p.vel=(SA1Vec3){0,below?2.99f:3.f,5};
        p.grounded=false;p.pc2004_action_state=10;p.pc2004_control_timer=1;
        step(&p,&pending,(SA1Input){.move_x=1,.jump_held=true});
        near(p.vel.z,below?4.86f:4.96f);assert(p.pc2004_control_timer==0);
    }
    p.vel=(SA1Vec3){0,0,0};p.pc2004_action_state=10;p.grounded=false;
    step(&p,&pending,(SA1Input){.jump_held=true});
    assert(p.pc2004_action_state==12&&p.animation_airborne_request==17);
    near(p.vel.y,-.08f);assert(p.jump_ticks==0);
    /* Flat movement stores yaw separately from the retained general frame.
     * A class1 launch preserves that current yaw, not stale reset axes. */
    sa1_player_reset(&p,sa3ds_movement_lab_world());p.yaw=1.5707963267948966f;
    placement.type=1;
    assert(sa3ds_spring_launch_prepare(&placement,&trig,(SA1Vec3){0},1,2,&launch));
    assert(sa3ds_spring_player_apply(&p,&placement,&trig,&launch,1,&pending));
    near(p.surface_forward.x,1);near(p.surface_forward.z,0);
    puts("Spring player bridge: PASS (deferred command, retained first ground bits, normal-force launch, timer, nocon equality, apex/fall without jump boost)");
    return 0;
}
