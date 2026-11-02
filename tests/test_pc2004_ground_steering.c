#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "movement.h"
#include "steam_surface.h"
#include "steam_flat.h"
#include "pc2004_motion.h"
#include "surface_frame.h"

static void near(float actual, float expected) {
    if (fabsf(actual - expected) > 2e-6f) {
        fprintf(stderr, "expected %.9g, got %.9g\n", expected, actual);
        assert(0);
    }
}
static SA1Surface plane(void *context, float x, float z) {
    SA1Vec3 n = *(SA1Vec3 *)context;
    return (SA1Surface){.ground_y=-(n.x*x+n.z*z)/n.y,
        .grad_x=-n.x/n.y,.grad_z=-n.z/n.y,.normal=n,.surface_id=0,.valid=true};
}
static SA1PlayerState player(SA1PhysicsWorld *world, SA1Vec3 n) {
    SA1PlayerState p;
    sa1_player_reset(&p, world);
    p.ground_normal=n;
    SA1SurfaceFrame f=sa1_surface_frame_make(n,(SA1Vec3){0,0,1});
    p.surface_forward=f.forward;p.surface_right=f.right;p.surface_id=0;
    p.pos=n;
    return p;
}
int main(void) {
    const SA1SteamPlayerParameters *params=&SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1PC2004LocalVector old={10,2,4}, transformed={0,-2,8}, v;
    v=sa1_pc2004_ground_turn_velocity(old,transformed,SA1_PC2004_TURN_GENERAL,1,1,1);
    near(v.forward,9.9f);near(v.normal,1.96f);near(v.side,4.04f);
    v=sa1_pc2004_ground_turn_velocity(old,transformed,SA1_PC2004_TURN_GENERAL,1,.4f,1);
    near(v.forward,5);near(v.normal,0);near(v.side,6);
    v=sa1_pc2004_ground_turn_velocity(old,transformed,SA1_PC2004_TURN_GENERAL,1,nextafterf(.4f,1),1);
    near(v.forward,9.9f);
    v=sa1_pc2004_ground_turn_velocity(old,transformed,SA1_PC2004_TURN_GENERAL,0,1,1);
    near(v.forward,.5f);near(v.normal,-1.8f);near(v.side,7.8f);
    v=sa1_pc2004_ground_turn_velocity(old,transformed,SA1_PC2004_TURN_GENERAL,1,1,.2f);
    near(v.forward,1.98f);near(v.normal,-1.208f);near(v.side,7.208f);
    v=sa1_pc2004_ground_turn_velocity(old,transformed,SA1_PC2004_TURN_SLOW,1,1,.2f);
    near(v.forward,9.9f);near(v.normal,1.96f);near(v.side,4.04f);
    v=sa1_pc2004_ground_turn_velocity(old,transformed,SA1_PC2004_TURN_QUICK,1,1,1);
    near(v.forward,0);near(v.normal,-2);near(v.side,8);
    assert(sa1_pc2004_slow_turn_step(params->dash_speed,params->dash_speed)==256);
    /* sqrt argument yields reduction ~10.71: source truncates10, old rounds11. */
    assert(sa1_pc2004_slow_turn_step(params->dash_speed+.028f,params->dash_speed)==246);

    SA1Vec3 n={0,1,0};
    SA1PhysicsWorld world={.query_surface=plane,.context=&n,.reset_position={0,1,0},
        .reset_min_x=-1000,.reset_max_x=1000,.reset_min_y=-1000,
        .reset_min_z=-1000,.reset_max_z=1000};
    SA1PlayerState flat=player(&world,n);
    flat.vel.z=params->dash_speed+.028f;
    SA1Input small_turn={.move_x=sinf(.1f),.move_z=cosf(.1f)};
    SA1SteamFlatDebug flat_dbg;
    sa1_physics_step_steam_sonic_flat(&flat,&small_turn,0,&world,
        &SA1_PHYSICS_PLACEHOLDER_CONSTANTS,&flat_dbg);
    assert(flat_dbg.turn_kind==SA1_STEAM_TURN_SLOW);
    assert(flat_dbg.turn_step_bams==246);

    SA1SteamSurfaceDebug dbg;
    SA1Input input={.move_x=1};
    const float theta=3.14159265358979323846f/16;
    float turned_forward=2*(.99f+.009999990463256836f*cosf(theta));
    SA1PlayerState p=player(&world,n);p.vel.z=2;
    sa1_physics_step_steam_sonic_surface(&p,&input,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,&dbg);
    assert(dbg.turn_step_bams==0x800);near(p.yaw,theta);
    near(dbg.forward_speed_after,turned_forward+params->slow_down);
    near(dbg.side_speed_after,0); /* Pre-turn side drag0; PGetSpeed snaps unchanged sub-jog side speed. */

    /* Quick turn preserves world velocity, re-expressing it in the new frame. */
    p=player(&world,n);p.vel.x=2;
    sa1_physics_step_steam_sonic_surface(&p,&input,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,&dbg);
    assert(dbg.turn_step_bams==0x2000);
    near(dbg.forward_speed_after,sqrtf(2));
    near(dbg.side_speed_after,sqrtf(2)+2*params->air_resist_z + params->grd_frict_z * -params->weight);

    /* Roll and charge both call GENERAL PGetRotation, never QUICK. */
    for(int charge=0;charge<=1;charge++) {
        p=player(&world,n);p.vel.z=2;p.ball_form=true;
        p.action_state=charge?SA1_ACTION_SPINDASH_CHARGE:SA1_ACTION_ROLL;
        input.action_held=charge;
        sa1_physics_step_steam_sonic_surface(&p,&input,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,&dbg);
        assert(dbg.turn_step_bams==0x800);near(p.yaw,theta);
        near(dbg.forward_speed_after-dbg.pc2004_add_forward,turned_forward);
        assert(charge?dbg.used_pc2004_pgetbreak:dbg.used_pc2004_pgetinertia);
    }
    input.action_held=false;
    /* Ordinary running's force vector is captured BEFORE steering. */
    n=(SA1Vec3){0,.8660254038f,-.5f};p=player(&world,n);
    p.vel=sa1_vec3_scale(p.surface_forward,2);
    sa1_physics_step_steam_sonic_surface(&p,&input,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,&dbg);
    near(dbg.tangent_gravity_forward,-params->weight*.5f);
    near(dbg.tangent_gravity_side,0);
    puts("PC2004 ground steering: PASS (mode/Up/friction blend, truncation, runtime run/quick/roll/charge, pre-turn force frame)");
    return 0;
}
