#include "../src/sa1/physics/pc2004_ground_accel.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static SA1PC2004GroundAccelInput ordinary(float a,float f,float v,int analog,int flag4){
    return (SA1PC2004GroundAccelInput){.force_after_resistance={a,-2,0},
        .input_force=f,.speed_after_turn={v,0,0},.up=1,.friction_multiplier=1,
        .analog_active=analog,.surface_flag4=flag4};
}
static void near(float value,float expected){assert(fabsf(value-expected)<1e-6f);}
int main(void){
    /* Synthetic binary-exact physics: K1, braking limit2, side limit3.
     * Hand-calculated witnesses distinguish branch order and asymmetries. */
    SA1SteamPlayerParameters p={.lim_frict=-.5f,.grd_frict=-1,
        .grd_frict_z=-1.5f,.jog_speed=2};
    const struct {float a,f,v;int analog,flag4;float forward,residual;int wrote;} cases[]={
        {0,3,0,1,0,3,0,0}, {0,.03125f,0,0,0,0,0,0},
        {0,1,0,0,0,1,0,0}, {0,-1,0,0,0,-1,0,0},
        {-.03125f,.015625f,-1,1,0,0,0,0},
        {-.03125f,.0625f,-1,1,0,.03125f,0,0},
        {0,3,1,1,0,1,2,1}, {0,1,1,1,0,1,0,0},
        {-3,3,1,1,0,0,0,0}, {4,-3,1,1,0,0,0,0},
        {1,-3,1,1,0,-2,0,0}, {50,-3,-1,1,0,-2,0,0},
        {50,-1,-1,1,0,0,0,0}, {0,-3,-1,1,1,-2,-5,1},
        {6,-3,1,1,1,2,1,1}, {4,-3,1,1,1,1,0,0},
        {0,-2,1,1,1,-2,0,0}, {.03125f,0,2,0,0,0,0,0},
        {.051f,0,2,0,0,.051f,0,0}, {-.051f,0,2,0,0,-.051f,0,0}
    };
    SA1PC2004GroundAccelResult out;
    for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
        SA1PC2004GroundAccelInput in=ordinary(cases[i].a,cases[i].f,cases[i].v,
            cases[i].analog,cases[i].flag4);
        assert(sa1_pc2004_ground_accel_clamp(&in,&p,&out));
        near(out.add_velocity.forward,cases[i].forward);
        near(out.animation_residual,cases[i].residual);
        assert(out.wrote_residual==cases[i].wrote);near(out.add_velocity.normal,-2);
    }
    SA1PC2004GroundAccelInput in=ordinary(.03125f,0,nextafterf(2,INFINITY),0,0);
    assert(sa1_pc2004_ground_accel_clamp(&in,&p,&out));near(out.add_velocity.forward,.03125f);
    in=ordinary(.25f,9,1,0,0);in.up=nextafterf(.71f,-INFINITY);
    in.force_after_resistance.side=.5f;
    assert(sa1_pc2004_ground_accel_clamp(&in,&p,&out));
    near(out.add_velocity.forward,2.5f);near(out.add_velocity.side,5);assert(!out.wrote_residual);
    in.up=.71f;assert(sa1_pc2004_ground_accel_clamp(&in,&p,&out));
    near(out.add_velocity.forward,1);near(out.animation_residual,8.25f);
    /* Same multiplication bypass must stop at the strict speed boundary. */
    in.up=0;in.speed_after_turn.forward=2;
    assert(sa1_pc2004_ground_accel_clamp(&in,&p,&out));near(out.add_velocity.forward,1);
    for(int flag=0;flag<=1;flag++){
        const float accelerations[]={-4,-2,0,2,4}, expected[]={-1,0,0,0,1};
        for(unsigned i=0;i<5;i++){
            in=ordinary(0,0,1,1,flag);in.speed_after_turn.side=7;
            in.force_after_resistance.side=accelerations[i];
            assert(sa1_pc2004_ground_accel_clamp(&in,&p,&out));
            near(out.add_velocity.side,expected[i]);
        }
    }
    in=ordinary(0,0,1,1,0);in.force_after_resistance.side=1;
    assert(sa1_pc2004_ground_accel_clamp(&in,&p,&out));near(out.add_velocity.side,1);
    in.force_after_resistance.side=nextafterf(1,0);
    assert(sa1_pc2004_ground_accel_clamp(&in,&p,&out));near(out.add_velocity.side,0);
    in.force_after_resistance.side=-1;
    assert(sa1_pc2004_ground_accel_clamp(&in,&p,&out));near(out.add_velocity.side,-1);
    in=ordinary(0,3,1,1,0);in.normal_addend=-10;
    assert(sa1_pc2004_ground_accel_clamp(&in,&p,&out));
    near(out.add_velocity.forward,1);near(out.add_velocity.normal,-12); /* clamps use N=-2 */
    in.force_after_resistance.normal=0;in.speed_after_turn.side=2;
    in.force_after_resistance.side=.25f;
    assert(sa1_pc2004_ground_accel_clamp(&in,&p,&out));near(out.add_velocity.side,.25f);
    /* Actual owner-independent parameter evidence: static upright normal is
     * -weight. First stopped tick differs from subsequent positive-speed tick. */
    const SA1SteamPlayerParameters *sonic=&SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    in=ordinary(0,sonic->run_accel,0,1,0);
    in.force_after_resistance.normal=-sonic->weight;
    assert(sa1_pc2004_ground_accel_clamp(&in,sonic,&out));
    near(out.add_velocity.forward,.05f);assert(!out.wrote_residual);
    in.speed_after_turn.forward=.05f;
    assert(sa1_pc2004_ground_accel_clamp(&in,sonic,&out));
    near(out.add_velocity.forward,.0226f);near(out.animation_residual,.0274f);
    assert(out.wrote_residual);
    in.friction_multiplier=.1f;
    assert(sa1_pc2004_ground_accel_clamp(&in,sonic,&out));
    near(out.add_velocity.forward,.00226f);near(out.animation_residual,.04774f);
    /* No stale residual on non-writing path, and invalid input is transactional. */
    in.speed_after_turn.forward=0;assert(sa1_pc2004_ground_accel_clamp(&in,sonic,&out));
    assert(!out.wrote_residual&&out.animation_residual==0);
    in.input_force=NAN;assert(!sa1_pc2004_ground_accel_clamp(&in,sonic,&out));
    assert(out.add_velocity.forward==0&&out.animation_residual==0&&!out.wrote_residual);
    /* PGetSpeed uses a side threshold, not a symmetric zero-cross guard. */
    SA1PC2004LocalVector next;
    p.jog_speed=2;p.run_speed=4;p.air_break=-.25f;
    assert(sa1_pc2004_pgetspeed_local((SA1PC2004LocalVector){1,2,.25f},
        (SA1PC2004LocalVector){-2,-.5f,-.5f},1,1,0,&p,&next));
    near(next.forward,0);near(next.normal,1.5f);near(next.side,-.25f);
    assert(sa1_pc2004_pgetspeed_local((SA1PC2004LocalVector){1,0,.25f},
        (SA1PC2004LocalVector){0},0,1,0,&p,&next));
    near(next.forward,0);near(next.side,0);
    float h=(float)((double)p.jog_speed*.87f);
    for(int sign=-1;sign<=1;sign+=2){
        assert(sa1_pc2004_pgetspeed_local((SA1PC2004LocalVector){0,0,sign*h},
            (SA1PC2004LocalVector){0,0,-sign*.125f},1,1,0,&p,&next));
        near(next.side,0);
        assert(sa1_pc2004_pgetspeed_local((SA1PC2004LocalVector){0,0,sign*h},
            (SA1PC2004LocalVector){0},1,1,0,&p,&next));
        assert(next.side==sign*h);
    }
    assert(sa1_pc2004_pgetspeed_local((SA1PC2004LocalVector){1,0,0},
        (SA1PC2004LocalVector){0},0,.7f,0,&p,&next));near(next.forward,1);
    assert(sa1_pc2004_pgetspeed_local((SA1PC2004LocalVector){3.5f,0,0},
        (SA1PC2004LocalVector){0},1,1,2,&p,&next));near(next.forward,3.25f);
    assert(sa1_pc2004_pgetspeed_local((SA1PC2004LocalVector){3,0,0},
        (SA1PC2004LocalVector){0},1,1,2,&p,&next));near(next.forward,2.75f);
    puts("PC2004 ground clamp: PASS (forward residual, side friction, strict boundaries and static-normal equations; no native parity claim)");
    return 0;
}
