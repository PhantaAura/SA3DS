#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "movement.h"
#include "steam_flat.h"
#include "steam_surface.h"
#include "surface_frame.h"
static SA1Vec3 normal;
static SA1Surface plane(void *ctx,float x,float z) {
    (void)ctx;
    return (SA1Surface){.ground_y=-(normal.x*x+normal.z*z)/normal.y,
        .grad_x=-normal.x/normal.y,.grad_z=-normal.z/normal.y,
        .normal=normal,.slope_radians=acosf(normal.y),.surface_id=0,.valid=true};
}
static SA1Contact ledge(void *ctx,SA1Vec3 pos,SA1Vec3 direction,float range) {
    (void)ctx;(void)direction;(void)range;
    if(pos.x>.1f)return (SA1Contact){0};
    return (SA1Contact){.point={pos.x,0,pos.z},.normal={0,1,0},
        .signed_distance=pos.y,.distance=fabsf(pos.y),.surface_id=0,.valid=true};
}
static SA1PlayerState setup(SA1PhysicsWorld *w) {
    SA1PlayerState p;sa1_player_reset(&p,w);
    p.pos=normal;p.ground_normal=normal;p.surface_id=0;
    SA1SurfaceFrame f=sa1_surface_frame_make(normal,(SA1Vec3){0,0,1});
    p.surface_forward=f.forward;p.surface_right=f.right;
    return p;
}
static void same(SA1Vec3 a,SA1Vec3 b) {
    float distance=sa1_vec3_length(sa1_vec3_sub(a,b));
    if(distance>1e-5f){fprintf(stderr,"frame dispatch delta %.9g\n",distance);assert(0);}
}
int main(void) {
    normal=(SA1Vec3){0,.8660254038f,-.5f};
    SA1PhysicsWorld w={.query_surface=plane,.reset_position={0,.8660254038f,-.5f},
        .reset_min_x=-1000,.reset_max_x=1000,.reset_min_y=-1000,
        .reset_min_z=-1000,.reset_max_z=1000};
    for(unsigned steer=0;steer<2;steer++) {
        SA1PlayerState p=setup(&w);
        SA1Input in={.jump_pressed=true,.jump_held=true};
        sa1_physics_step_steam_sonic_flat(&p,&in,0,&w,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
        assert(!p.grounded&&p.surface_frame_active);
        SA1PlayerState reference=p;
        for(unsigned tick=0;tick<8;tick++) {
            in=(SA1Input){.jump_held=tick<3,.move_x=steer?.5f:0,.move_z=steer?.5f:0};
            sa1_physics_step_steam_sonic_flat(&p,&in,0,&w,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
            sa1_physics_step_steam_sonic_surface(&reference,&in,0,&w,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
            assert(!p.grounded&&p.surface_frame_active);
            same(p.vel,reference.vel);same(p.pos,reference.pos);
            same(p.surface_forward,reference.surface_forward);
        }
    }
    normal=(SA1Vec3){0,1,0};w.query_contact=ledge;
    SA1PlayerState p=setup(&w);p.vel.z=2;
    SA1Input in={.move_x=1};
    sa1_physics_step_steam_sonic_surface(&p,&in,0,&w,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
    assert(!p.grounded&&p.surface_frame_active);
    assert(p.yaw>.1f&&p.surface_forward.x>.1f); /* Turn survives missed support. */
    w.query_contact=NULL;
    p.grounded=true;p.vel=(SA1Vec3){0};p.pos=(SA1Vec3){0,1,0};in=(SA1Input){0};
    sa1_physics_step_steam_sonic_flat(&p,&in,0,&w,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
    assert(!p.surface_frame_active);
    sa1_player_reset(&p,&w);assert(!p.surface_frame_active);

    /* Sonic_Act1 action12 never calls the action8 held-jump selector. A
     * held button while falling must not become a new .8*jmp_addit boost. */
    for(unsigned general=0;general<2;general++) {
        normal=(SA1Vec3){0,1,0};
        SA1PlayerState held=setup(&w),released;
        held.pos.y=20;held.vel.y=-0.5f;held.grounded=false;
        held.action_state=SA1_ACTION_AIRBORNE;
        held.pc2004_action_state=12;held.pc2004_action_state_valid=true;
        held.surface_frame_active=general!=0;released=held;
        SA1Input hold={.jump_held=true},none={0};
        sa1_physics_step_steam_sonic_flat(&held,&hold,0,&w,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
        sa1_physics_step_steam_sonic_flat(&released,&none,0,&w,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
        same(held.vel,released.vel);same(held.pos,released.pos);
        assert(held.jump_ticks==0);
    }

    /* Flat and general entry points must both use action4/5->PGetRotation,
     * then break/inertia. The old flat public path still used direct turning. */
    for(unsigned charge=0;charge<2;charge++) {
        SA1PlayerState flat=setup(&w);flat.vel.z=2;flat.ball_form=true;
        flat.action_state=charge?SA1_ACTION_SPINDASH_CHARGE:SA1_ACTION_ROLL;
        flat.pc2004_action_state=charge?4:5;flat.pc2004_action_state_valid=true;
        SA1PlayerState general=flat;
        in=(SA1Input){.move_x=1,.action_held=charge};
        for(unsigned tick=0;tick<4;tick++) {
            sa1_physics_step_steam_sonic_flat(&flat,&in,0,&w,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
            sa1_physics_step_steam_sonic_surface(&general,&in,0,&w,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
            same(flat.vel,general.vel);same(flat.pos,general.pos);
            assert(fabsf(flat.yaw-general.yaw)<1e-5f);
            assert(flat.action_state==general.action_state);
            assert(flat.spindash_ticks==general.spindash_ticks);
        }
    }
    puts("Player frame dispatch: PASS (multi-tick slope jump/hold/steer, ledge turn retention, flat/reset clearing, flat/general charge+roll)");
    return 0;
}
