#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "pc2004_motion.h"
#include "steam_player_parameters.h"
#include "surface_frame.h"

static int nearf(float a, float b, float eps) { return fabsf(a - b) <= eps; }

int main(void) {
    const SA1SteamPlayerParameters *p = &SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA1PC2004MotionContext ctx = {1, 0, 0, 1.0f, 1.0f};
    SA1PC2004MotionDebug dbg;

    /* Flat, supported Sonic charging from +5 local speed.
       base drag=-.04, run_break=-.18, grounded friction=.008 => -.212. */
    SA1PC2004LocalVector speed = {5.0f, 0.0f, 0.0f};
    SA1PC2004LocalVector force = {0.0f, -p->weight, 0.0f};
    SA1PC2004LocalVector add = sa1_pc2004_pgetbreak_add_velocity(
        speed, force, &ctx, p, &dbg);
    assert(nearf(add.forward, -0.212f, 1e-6f));
    assert(nearf(add.normal, -p->weight, 1e-6f));
    assert(dbg.used_ground_friction);

    /* Zero-speed PGetBreak uses lim_frict * local normal force as the deadband. */
    speed = (SA1PC2004LocalVector){0.0f, 0.0f, 0.0f};
    force = (SA1PC2004LocalVector){0.01f, -p->weight, 0.0f};
    add = sa1_pc2004_pgetbreak_add_velocity(speed, force, &ctx, p, &dbg);
    assert(nearf(add.forward, 0.0f, 1e-7f));
    assert(dbg.used_zero_speed_lim_frict);

    /* Negative forward speed gets braking with the opposite sign. */
    speed = (SA1PC2004LocalVector){-1.0f, 0.0f, 0.0f};
    force = (SA1PC2004LocalVector){0.0f, -p->weight, 0.0f};
    add = sa1_pc2004_pgetbreak_add_velocity(speed, force, &ctx, p, &dbg);
    assert(nearf(add.forward, 0.180f, 1e-6f));

    /* Break AddVelocity is clamped if it would reverse the current speed. */
    speed = (SA1PC2004LocalVector){0.1f, 0.0f, 0.0f};
    force = (SA1PC2004LocalVector){-10.0f, -p->weight, 0.0f};
    add = sa1_pc2004_pgetbreak_add_velocity(speed, force, &ctx, p, &dbg);
    assert(nearf(add.forward, -0.1f, 1e-6f));
    assert(dbg.clamped_forward_zero_cross);

    /* Ordinary PGetInertia flat specialization: local force + roll drag. */
    speed = (SA1PC2004LocalVector){5.0f, 0.0f, 0.0f};
    force = (SA1PC2004LocalVector){0.0f, -p->weight, 0.0f};
    add = sa1_pc2004_pgetinertia_add_velocity(speed, force, &ctx, p, &dbg);
    assert(nearf(add.forward, p->air_resist * 5.0f, 1e-6f));
    assert(nearf(add.normal, -p->weight, 1e-6f));

    /* 0x4436F7 uses stack+0xC, the normal force subsequently stored at
     * AddVelocity+4. Forward uses stack+8 and must not receive this factor. */
    ctx.up_y = -0.2f;
    speed = (SA1PC2004LocalVector){3.0f, 0.0f, 0.0f};
    force = (SA1PC2004LocalVector){0.5f, -0.02f, 0.0f};
    add = sa1_pc2004_pgetinertia_add_velocity(speed, force, &ctx, p, &dbg);
    assert(nearf(add.forward, 0.5f + p->air_resist * 3.0f, 1e-6f));
    assert(nearf(add.normal, 0.16f, 1e-6f));
    assert(dbg.used_inertia_negative_up_flip);

    /* Independent component equations, including exact run-speed boundary,
     * signed velocity, signed zero/near-zero Up and both ball-status paths.
     * Synthetic local frames: no retail inverted-contact trace is implied. */
    const float ups[]={-1.0f,-0.000001f,-0.0f,0.0f,0.000001f,0.5f,1.0f};
    const float forwards[]={-5.0f,-0.1f,0.0f,
        nextafterf(p->run_speed,-INFINITY),p->run_speed,
        nextafterf(p->run_speed,INFINITY),5.0f};
    unsigned inverted_cases=0;
    for(unsigned u=0;u<sizeof(ups)/sizeof(*ups);u++)
    for(unsigned v=0;v<sizeof(forwards)/sizeof(*forwards);v++)
    for(unsigned ball=0;ball<2;ball++){
        ctx.up_y=ups[u];ctx.status_0x100=(int)ball;
        speed=(SA1PC2004LocalVector){forwards[v],-0.25f,-0.75f};
        force=(SA1PC2004LocalVector){0.5f,-0.02f,0.3f};
        int flip=forwards[v]>p->run_speed&&ups[u]<0.0f;
        float drag=ball&&(double)ups[u]<=0.98 ? -0.00019999999494757503f : p->air_resist;
        SA1PC2004LocalVector expected={
            force.forward+drag*speed.forward,
            force.normal*(flip?-8.0f:1.0f)+p->air_resist_y*speed.normal,
            force.side+p->air_resist_z*speed.side};
        add=sa1_pc2004_pgetinertia_add_velocity(speed,force,&ctx,p,&dbg);
        assert(dbg.used_inertia_negative_up_flip==flip);
        assert(nearf(add.forward,expected.forward,1e-6f));
        assert(nearf(add.normal,expected.normal,1e-6f));
        assert(nearf(add.side,expected.side,1e-6f));
        /* PGetSpeed+0x444031 integrates normal directly (no X/Z stop band). */
        assert(isfinite(speed.normal+add.normal));
        assert(isfinite(sa1_pc2004_integrate_axis_no_cross(speed.forward,add.forward,NULL)));
        inverted_cases++;
    }
    assert(inverted_cases==98);
    puts("PC2004 inverted-Up normal-force equations: PASS (98 synthetic local-frame cases)");

    /* Fixed synthetic inverted contact frame: normal is world -Y, while
     * forward remains world +Z. Check direct normal integration over120
     * ticks against the independent closed-form drag recurrence. */
    SA1SurfaceFrame inverted=sa1_surface_frame_make(
        (SA1Vec3){0,-1,0},(SA1Vec3){0,0,1});
    assert(inverted.valid);
    ctx.up_y=-1;ctx.status_0x100=1;
    speed=(SA1PC2004LocalVector){5,0,0};
    force=(SA1PC2004LocalVector){0,-0.02f,0};
    SA1Vec3 position={0,0,0};
    float expected_position_y=0;
    for(unsigned tick=1;tick<=120;tick++){
        add=sa1_pc2004_pgetinertia_add_velocity(speed,force,&ctx,p,&dbg);
        speed.forward=sa1_pc2004_integrate_axis_no_cross(speed.forward,add.forward,NULL);
        speed.normal+=add.normal;
        speed.side=sa1_pc2004_integrate_axis_no_cross(speed.side,add.side,NULL);
        float expected_normal=0.16f*(1.0f-powf(1.0f+p->air_resist_y,(float)tick))/-p->air_resist_y;
        assert(nearf(speed.normal,expected_normal,0.0001f));
        SA1Vec3 world=sa1_vec3_add(sa1_vec3_scale(inverted.forward,speed.forward),
            sa1_vec3_add(sa1_vec3_scale(inverted.up,speed.normal),sa1_vec3_scale(inverted.right,speed.side)));
        position=sa1_vec3_add(position,world);
        expected_position_y-=expected_normal;
        assert(isfinite(position.x)&&isfinite(position.y)&&isfinite(position.z));
        assert(nearf(position.y,expected_position_y,0.01f));
        assert(world.y<0&&world.z>0&&dbg.used_inertia_negative_up_flip);
    }
    puts("PC2004 inverted-Up integration: PASS (120 synthetic-frame ticks; no retail contact claim)");

    /* status 0x100 uses the exact 0.0002 forward drag branch at Up <= .98. */
    ctx.up_y = 0.5f;
    ctx.status_0x100 = 1;
    speed = (SA1PC2004LocalVector){5.0f, 0.0f, 0.0f};
    force = (SA1PC2004LocalVector){1.0f, 0.0f, 0.0f};
    add = sa1_pc2004_pgetinertia_add_velocity(speed, force, &ctx, p, &dbg);
    assert(nearf(add.forward, 0.999f, 1e-6f));
    assert(dbg.used_inertia_status_0x100_drag);

    /* Up is stored f32 but compared against retail's f64 0.98 constant.
     * Rounded 0.98f lies ABOVE that constant and uses ordinary resistance. */
    ctx.up_y = 0.98f;
    add = sa1_pc2004_pgetinertia_add_velocity(speed, force, &ctx, p, &dbg);
    assert(!dbg.used_inertia_status_0x100_drag);
    assert(nearf(add.forward, 0.96f, 1e-6f));
    ctx.up_y = nextafterf(0.98f, -INFINITY);
    add = sa1_pc2004_pgetinertia_add_velocity(speed, force, &ctx, p, &dbg);
    assert(dbg.used_inertia_status_0x100_drag);
    assert(nearf(add.forward, 0.999f, 1e-6f));
    ctx.status_0x100 = 0;
    add = sa1_pc2004_pgetinertia_add_velocity(speed, force, &ctx, p, &dbg);
    assert(!dbg.used_inertia_status_0x100_drag);
    assert(nearf(add.forward, 0.96f, 1e-6f));

    /* PGetBreak never tests status 0x100; charging arithmetic is unchanged. */
    ctx.up_y=0.5f;
    SA1PC2004LocalVector without_ball=sa1_pc2004_pgetbreak_add_velocity(
        speed,force,&ctx,p,NULL);
    ctx.status_0x100=1;
    SA1PC2004LocalVector with_ball=sa1_pc2004_pgetbreak_add_velocity(
        speed,force,&ctx,p,NULL);
    assert(without_ball.forward==with_ball.forward);
    assert(without_ball.normal==with_ball.normal);
    assert(without_ball.side==with_ball.side);

    int clamped = 0;
    assert(nearf(sa1_pc2004_integrate_axis_no_cross(0.25f, -0.5f, &clamped),
                 0.0f, 1e-7f));
    assert(clamped);
    assert(nearf(sa1_pc2004_integrate_axis_no_cross(1.0f, -0.2f, &clamped),
                 0.8f, 1e-7f));
    assert(!clamped);

    printf("PC 2004 PGetBreak/PGetInertia arithmetic tests: PASS\n");
    return 0;
}
