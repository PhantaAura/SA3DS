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
     * plane through the selected point with the caller's task Up normal.
     * The later body-offset addition and subtraction cancel
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

float sa1_pc2004_collision_prepass_length(
    SA1Vec3 up, SA1Vec3 gravity, SA1Vec3 motion, int supported,
    const SA1PC2004CollisionCandidate *candidates, size_t count) {
    float length = sqrtf(motion.x * motion.x + motion.y * motion.y + motion.z * motion.z);
    if (length == 0.0f) return length;
    float closest = 1000.0f;
    int found = 0;
    for (size_t i = 0; i < count; i++) {
        const SA1PC2004CollisionCandidate *candidate = &candidates[i];
        int ground_side = sa1_pc2004_collision_ground_side(up, candidate->normal, gravity, 0);
        double inward = (double)motion.x * candidate->normal.x +
            (double)motion.y * candidate->normal.y +
            (double)motion.z * candidate->normal.z;
        if (supported ? (!ground_side && inward < 0.0) : ground_side) {
            if (candidate->corrected_distance < closest) {
                closest = candidate->corrected_distance;
                found = 1;
            }
        }
    }
    return found && closest < length ? closest : length;
}

void sa1_pc2004_collision_response_order(size_t count, size_t promoted,
    size_t previous, int moving_support, size_t *order) {
    size_t next = 0;
    if (promoted < count) order[next++] = promoted;
    for (size_t i = 0; i < count; i++) {
        size_t index = moving_support ? i : count - 1 - i;
        if (index != promoted && index != previous) order[next++] = index;
    }
    if (previous < count && previous != promoted) order[next] = previous;
}

SA1Vec3 sa1_pc2004_fast_ground_probe_center(SA1Vec3 origin, SA1Vec3 up,
    float height, float local_vertical_speed, float pos_error) {
    if (local_vertical_speed <= 0.0f) height -= pos_error;
    return (SA1Vec3){
        origin.x + up.x * height,
        origin.y + up.y * height,
        origin.z + up.z * height,
    };
}

int sa1_pc2004_collision_support_speed(SA1Vec3 travel, SA1Vec3 normal,
    float run_speed, float local_vertical_speed, unsigned low_ground_bits,
    uint32_t flags) {
    if (local_vertical_speed > 0.0f && low_ground_bits != 0) return 0;

    double length = sqrt((double)travel.x * travel.x +
        (double)travel.y * travel.y + (double)travel.z * travel.z);
    if (length == 0.0 || length < run_speed || (flags & 0x4000u) != 0) return 1;

    double alignment = (double)(float)(travel.x / length) * normal.x +
        (double)(float)(travel.y / length) * normal.y +
        (double)(float)(travel.z / length) * normal.z;
    return alignment <= 0.52;
}
