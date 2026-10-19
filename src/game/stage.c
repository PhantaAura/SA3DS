#include "stage.h"
#include "../sa1/physics/surface_frame.h"
#include <math.h>
#include <string.h>

#define LAB_LOOP_CENTER_Y 18.0f
#define LAB_LOOP_CENTER_Z 225.0f
#define LAB_LOOP_RADIUS   14.0f
#define LAB_LOOP_HALF_W   10.5f
#define LAB_LOOP_FLAG     0x00000001u

SA1Surface sa3ds_movement_lab_surface(void *context, float x, float z) {
    (void)context;
    SA1Surface surface = {0};
    float half = 14.0f;
    if (z < -48.0f || z > 240.0f) return surface;

    if (z < 48.0f) {
        surface.surface_id = 1;
        if (fabsf(x) > half) return surface;
    } else if (z < 82.0f) {
        surface.surface_id = 2;
        if (fabsf(x) > half) return surface;
        surface.grad_z = 5.0f / 34.0f;
        surface.ground_y = (z - 48.0f) * surface.grad_z;
    } else if (z < 108.0f) {
        surface.surface_id = 3;
        half = 13.0f;
        if (fabsf(x) > half) return surface;
        surface.grad_z = 11.0f / 26.0f;
        surface.ground_y = 5.0f + (z - 82.0f) * surface.grad_z;
    } else if (z < 138.0f) {
        surface.surface_id = 4;
        half = 12.0f;
        if (fabsf(x) > half) return surface;
        surface.ground_y = 16.0f;
    } else if (z < 184.0f) {
        surface.surface_id = 5;
        half = 16.0f;
        if (fabsf(x) > half) return surface;
        float t = (z - 138.0f) / 46.0f;
        float bank = -3.0f + 6.0f * t;
        surface.ground_y = 16.0f + (x / half) * bank;
        surface.grad_x = bank / half;
    } else if (z < 210.0f) {
        surface.surface_id = 6;
        half = 13.0f;
        if (fabsf(x) > half) return surface;
        surface.grad_z = -12.0f / 26.0f;
        surface.ground_y = 16.0f + (z - 184.0f) * surface.grad_z;
    } else {
        /* Flat approach beneath the general-contact loop rig. The old
         * heightfield renderer can still draw this floor; loop support itself
         * comes from sa3ds_movement_lab_contact(). */
        surface.surface_id = 7;
        half = 16.0f;
        if (fabsf(x) > half) return surface;
        surface.ground_y = 4.0f;
    }

    surface.normal = sa1_surface_normal(surface.grad_x, surface.grad_z);
    surface.slope_radians = sa1_surface_slope_radians(surface.normal);
    surface.valid = true;
    return surface;
}

static SA1Contact heightfield_contact(SA1Vec3 position,
                                      SA1Vec3 probe_direction,
                                      float max_distance) {
    SA1Contact c;
    memset(&c, 0, sizeof(c));
    c.surface_id = -1;
    SA1Surface s = sa3ds_movement_lab_surface(0, position.x, position.z);
    if (!s.valid) return c;
    /* The visual flat guide continues beneath the loop, but it is not a
     * support surface after the entry tangent; otherwise the floor would win
     * the nearest-contact query and prevent orientation from rotating onto the
     * curved rig. */
    if (fabsf(position.x) <= LAB_LOOP_HALF_W &&
        position.z > LAB_LOOP_CENTER_Z + 0.05f)
        return c;

    c.point = (SA1Vec3){position.x, s.ground_y, position.z};
    c.normal = sa1_vec3_normalize(s.normal);
    SA1Vec3 delta = sa1_vec3_sub(position, c.point);
    c.signed_distance = sa1_vec3_dot(delta, c.normal);
    c.distance = fabsf(c.signed_distance);
    c.surface_id = s.surface_id;

    SA1Vec3 probe = sa1_vec3_normalize(probe_direction);
    float alignment = sa1_vec3_dot(probe, sa1_vec3_scale(c.normal, -1.0f));
    c.valid = c.distance <= max_distance && c.signed_distance >= -0.15f &&
              alignment > 0.15f;
    return c;
}

static SA1Contact loop_contact(SA1Vec3 position,
                               SA1Vec3 probe_direction,
                               float max_distance) {
    SA1Contact c;
    memset(&c, 0, sizeof(c));
    c.surface_id = -1;
    if (fabsf(position.x) > LAB_LOOP_HALF_W) return c;

    float ry = position.y - LAB_LOOP_CENTER_Y;
    float rz = position.z - LAB_LOOP_CENTER_Z;
    float radial_len = sqrtf(ry*ry + rz*rz);
    if (radial_len < LAB_LOOP_RADIUS - 4.0f ||
        radial_len > LAB_LOOP_RADIUS + 4.0f || radial_len < 1e-5f)
        return c;

    SA1Vec3 outward = {0.0f, ry / radial_len, rz / radial_len};
    c.point = (SA1Vec3){position.x,
                        LAB_LOOP_CENTER_Y + outward.y * LAB_LOOP_RADIUS,
                        LAB_LOOP_CENTER_Z + outward.z * LAB_LOOP_RADIUS};
    /* Player runs on the inside of the ring, so support normal points inward. */
    c.normal = sa1_vec3_scale(outward, -1.0f);
    SA1Vec3 delta = sa1_vec3_sub(position, c.point);
    c.signed_distance = sa1_vec3_dot(delta, c.normal);
    c.distance = fabsf(c.signed_distance);
    c.surface_id = 100;
    c.surface_flags = LAB_LOOP_FLAG;

    SA1Vec3 probe = sa1_vec3_normalize(probe_direction);
    float alignment = sa1_vec3_dot(probe, sa1_vec3_scale(c.normal, -1.0f));
    c.valid = c.distance <= max_distance && c.signed_distance >= -0.15f &&
              alignment > 0.20f;
    return c;
}

static float contact_score(SA1Contact c, SA1Vec3 probe_direction) {
    if (!c.valid) return 1e30f;
    SA1Vec3 probe = sa1_vec3_normalize(probe_direction);
    float alignment = sa1_vec3_dot(probe, sa1_vec3_scale(c.normal, -1.0f));
    /* Prefer a contact whose surface is one support offset from the player and
     * whose normal matches the current probe direction. */
    return fabsf(c.signed_distance - 1.0f) + (1.0f - alignment) * 0.45f;
}

SA1Contact sa3ds_movement_lab_contact(void *context, SA1Vec3 position,
                                      SA1Vec3 probe_direction,
                                      float max_distance) {
    (void)context;
    SA1Contact floor = heightfield_contact(position, probe_direction, max_distance);
    SA1Contact loop = loop_contact(position, probe_direction, max_distance);
    float floor_score = contact_score(floor, probe_direction);
    float loop_score = contact_score(loop, probe_direction);

    /* A tiny preference at/after the bottom tangent prevents the duplicate flat
     * approach plane from stealing support from the curved rig. This is
     * Movement-Lab fixture arbitration, not a claimed retail collision rule. */
    if (loop.valid && position.z >= LAB_LOOP_CENTER_Z)
        loop_score -= 0.02f;
    return loop_score < floor_score ? loop : floor;
}

const SA1PhysicsWorld *sa3ds_movement_lab_world(void) {
    static SA1PhysicsWorld world = {
        .query_surface = sa3ds_movement_lab_surface,
        .query_contact = sa3ds_movement_lab_contact,
        .context = 0,
        .reset_position = {0.0f, 1.0f, -32.0f},
        .reset_min_x = -90.0f,
        .reset_max_x = 90.0f,
        .reset_min_y = -35.0f,
        .reset_min_z = -80.0f,
        .reset_max_z = 280.0f
    };
    return &world;
}
