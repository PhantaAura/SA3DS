#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "pc2004_motion.h"
#include "steam_player_parameters.h"

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

    /* Exact negative-Up/high-speed branch multiplies local forward force by -8. */
    ctx.up_y = -0.2f;
    speed = (SA1PC2004LocalVector){3.0f, 0.0f, 0.0f};
    force = (SA1PC2004LocalVector){0.5f, -0.02f, 0.0f};
    add = sa1_pc2004_pgetinertia_add_velocity(speed, force, &ctx, p, &dbg);
    assert(nearf(add.forward, -4.0f + p->air_resist * 3.0f, 1e-6f));
    assert(dbg.used_inertia_negative_up_flip);

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
