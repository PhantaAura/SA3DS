#include "pc2004_collision_work.h"

#include <math.h>
#include <string.h>

void sa1_pc2004_collision_work_init(
    SA1PC2004CollisionWork *work,
    const SA1SteamPlayerParameters *p,
    SA1Vec3 pos,
    SA1Vec3 speed,
    int32_t angx,
    int32_t angz) {
    if (!work || !p) return;
    memset(work, 0, sizeof(*work));
    work->radius = p->height * 0.5f;
    work->pos = pos;
    work->spd = speed;
    work->tnorm = (SA1Vec3){0.0f, 1.0f, 0.0f};
    work->angx = angx;
    work->angz = angz;
}

void sa1_pc2004_collision_work_mirror_static_contact(
    SA1PC2004CollisionWork *work,
    const SA1Contact *contact,
    uint16_t selected_index) {
    if (!work || !contact || !contact->valid) return;
    work->find_count = 1;
    work->selected_nmb = selected_index;
    work->normal = contact->normal;
    work->onpoly = contact->point;
}

int sa1_pc2004_collision_ground_side(SA1Vec3 up, SA1Vec3 normal, SA1Vec3 gravity,
                                     int force_ground_side) {
    /* 438410 stores the dot as a float before acos, then truncates BAMS. */
    float alignment = (float)((double)up.x * normal.x +
        (double)up.y * normal.y + (double)up.z * normal.z);
    if (!isfinite(alignment)) return 0;
    if (alignment >= 1.0f) return 1;
    if (alignment > -1.0f) {
        double angle = acos((double)alignment) * (65536.0 * 0.15915497620314795);
        if (angle < 0x2c01) return 1;
    }
    float old_gravity_dot = (float)((double)gravity.x * up.x +
        (double)gravity.y * up.y + (double)gravity.z * up.z);
    double candidate_gravity_dot = (double)gravity.x * normal.x +
        (double)gravity.y * normal.y + (double)gravity.z * normal.z;
    return force_ground_side || candidate_gravity_dot < old_gravity_dot;
}

SA1Vec3 sa1_pc2004_collision_support_position(SA1Vec3 corrected_center,
    SA1Vec3 tangent_step, SA1Vec3 plane_point, SA1Vec3 plane_normal) {
    /* 788D40 projects the center plus tangential displacement onto the
     * selected face. The later body-offset addition and subtraction cancel
     * for static support before the task-position write. */
    SA1Vec3 end = {
        corrected_center.x + tangent_step.x,
        corrected_center.y + tangent_step.y,
        corrected_center.z + tangent_step.z,
    };
    float offset =
        (end.x - plane_point.x) * plane_normal.x +
        (end.y - plane_point.y) * plane_normal.y +
        (end.z - plane_point.z) * plane_normal.z;
    float normal_squared =
        plane_normal.x * plane_normal.x +
        plane_normal.y * plane_normal.y +
        plane_normal.z * plane_normal.z;
    float distance = offset / normal_squared;
    return (SA1Vec3){
        end.x - distance * plane_normal.x,
        end.y - distance * plane_normal.y,
        end.z - distance * plane_normal.z,
    };
}
