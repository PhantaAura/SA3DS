#include "pc2004_motion.h"

#include <math.h>
#include <string.h>

#define PC2004_INERTIA_NEGATIVE_UP_FORWARD_SCALE (-8.0f)
#define PC2004_INERTIA_STATUS_0X100_DRAG (0.00019999999494757503f)
/* Retail uses an f64 memory comparison, not a rounded f32 threshold. */
#define PC2004_INERTIA_STATUS_0X100_UP_LIMIT (0.98)

static float clamp_break_axis(float speed,
                              float accel,
                              float normal_force,
                              float axis_friction,
                              float acceleration_multiplier,
                              float lim_frict,
                              float run_break,
                              int *used_zero_speed_lim_frict,
                              int *used_ground_friction,
                              int *clamped_zero_cross) {
    if (speed == 0.0f) {
        /* Exact zero-speed branch: threshold = lim_frict * multiplier *
         * local normal force; only a positive threshold can form the retail
         * interval (-threshold, +threshold) that is snapped to zero. */
        float threshold = lim_frict * acceleration_multiplier * normal_force;
        if (threshold > 0.0f && accel < threshold && accel > -threshold) {
            accel = 0.0f;
            if (used_zero_speed_lim_frict) *used_zero_speed_lim_frict = 1;
        }
        return accel;
    }

    /* run_break itself is negative for Sonic. PGetBreak applies it with the
     * sign opposite current speed. */
    if (speed > 0.0f)
        accel += run_break * acceleration_multiplier;
    else
        accel -= run_break * acceleration_multiplier;

    /* Ground-friction tail is only produced when the local normal force is
     * negative. Sonic's grd_frict values are negative, so the amount is a
     * positive scalar that moves AddVelocity toward zero. */
    if (normal_force < 0.0f) {
        float amount = axis_friction * acceleration_multiplier * normal_force;
        if (amount > 0.0f) {
            if (accel > 0.0f) {
                accel -= amount;
                if (accel < 0.0f) accel = 0.0f;
            } else if (accel < 0.0f) {
                accel += amount;
                if (accel > 0.0f) accel = 0.0f;
            }
            if (used_ground_friction) *used_ground_friction = 1;
        }
    }

    /* 0x449065..0x4490B9 / 0x449165..0x4491B2: do not allow this
     * AddVelocity contribution to reverse the corresponding local speed. */
    if (speed > 0.0f && accel < -speed) {
        accel = -speed;
        if (clamped_zero_cross) *clamped_zero_cross = 1;
    } else if (speed < 0.0f && accel > -speed) {
        accel = -speed;
        if (clamped_zero_cross) *clamped_zero_cross = 1;
    }
    return accel;
}

SA1PC2004LocalVector sa1_pc2004_pgetbreak_add_velocity(
    SA1PC2004LocalVector speed,
    SA1PC2004LocalVector local_force,
    const SA1PC2004MotionContext *context,
    const SA1SteamPlayerParameters *p,
    SA1PC2004MotionDebug *debug_out) {
    SA1PC2004MotionDebug local_debug;
    SA1PC2004MotionDebug *dbg = debug_out ? debug_out : &local_debug;
    memset(dbg, 0, sizeof(*dbg));

    if (!context || !p || context->status_0x4000)
        return (SA1PC2004LocalVector){0.0f, 0.0f, 0.0f};

    SA1PC2004LocalVector out = local_force;
    out.forward += p->air_resist * speed.forward;
    out.normal += p->air_resist_y * speed.normal;
    out.side += p->air_resist_z * speed.side;

    if (context->grounded_status) {
        float mult = context->acceleration_multiplier;
        out.forward = clamp_break_axis(
            speed.forward, out.forward, out.normal,
            p->grd_frict, mult, p->lim_frict, p->run_break,
            &dbg->used_zero_speed_lim_frict,
            &dbg->used_ground_friction,
            &dbg->clamped_forward_zero_cross);
        out.side = clamp_break_axis(
            speed.side, out.side, out.normal,
            p->grd_frict_z, mult, p->lim_frict, p->run_break,
            &dbg->used_zero_speed_lim_frict,
            &dbg->used_ground_friction,
            &dbg->clamped_side_zero_cross);
    }

    return out;
}

SA1PC2004LocalVector sa1_pc2004_pgetinertia_add_velocity(
    SA1PC2004LocalVector speed,
    SA1PC2004LocalVector local_force,
    const SA1PC2004MotionContext *context,
    const SA1SteamPlayerParameters *p,
    SA1PC2004MotionDebug *debug_out) {
    SA1PC2004MotionDebug local_debug;
    SA1PC2004MotionDebug *dbg = debug_out ? debug_out : &local_debug;
    memset(dbg, 0, sizeof(*dbg));

    if (!context || !p || context->status_0x4000)
        return (SA1PC2004LocalVector){0.0f, 0.0f, 0.0f};

    SA1PC2004LocalVector out = local_force;

    /* 0x4436D4..0x443705: if forward speed is above run_speed while Up is
     * negative, the local forward force is multiplied by -8.0f. */
    if (speed.forward > p->run_speed && context->up_y < 0.0f) {
        out.forward *= PC2004_INERTIA_NEGATIVE_UP_FORWARD_SCALE;
        dbg->used_inertia_negative_up_flip = 1;
    }

    /* The exact routine has a grounded pending-collision impulse consumer at
     * 0x44370B..0x44376E. The Movement Lab does not produce that opaque buffer,
     * so the zero-pending-impulse specialization falls through here. */

    if (context->status_0x100 && context->up_y <= PC2004_INERTIA_STATUS_0X100_UP_LIMIT) {
        out.forward -= speed.forward * PC2004_INERTIA_STATUS_0X100_DRAG;
        dbg->used_inertia_status_0x100_drag = 1;
    } else {
        out.forward += p->air_resist * speed.forward;
    }
    out.normal += p->air_resist_y * speed.normal;
    out.side += p->air_resist_z * speed.side;
    return out;
}

float sa1_pc2004_integrate_axis_no_cross(float speed, float add_velocity,
                                         int *clamped_out) {
    float next = speed + add_velocity;
    int clamped = 0;
    if ((speed > 0.0f && next < 0.0f) ||
        (speed < 0.0f && next > 0.0f)) {
        next = 0.0f;
        clamped = 1;
    }
    if (clamped_out) *clamped_out = clamped;
    return next;
}
