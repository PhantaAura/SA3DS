#include "ground.h"
#include <math.h>
#include <string.h>

static float dot3(SA1Vec3 a, SA1Vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static float length3(SA1Vec3 v) {
    return sqrtf(dot3(v, v));
}

static SA1Vec3 normalize3(SA1Vec3 v) {
    float len = length3(v);
    if (len > 1e-8f) {
        v.x /= len; v.y /= len; v.z /= len;
    }
    return v;
}

SA1Vec3 sa1_surface_normal(float grad_x, float grad_z) {
    SA1Vec3 n = {-grad_x, 1.0f, -grad_z};
    return normalize3(n);
}

float sa1_surface_slope_radians(SA1Vec3 normal) {
    float y = normal.y;
    if (y < -1.0f) y = -1.0f;
    if (y > 1.0f) y = 1.0f;
    return acosf(y);
}

SA1Contact sa1_world_query_contact(const SA1PhysicsWorld *world,
                                   SA1Vec3 position,
                                   SA1Vec3 probe_direction,
                                   float max_distance) {
    SA1Contact contact;
    memset(&contact, 0, sizeof(contact));
    contact.surface_id = -1;

    if (!world) return contact;
    if (world->query_contact)
        return world->query_contact(world->context, position,
                                    probe_direction, max_distance);
    if (!world->query_surface) return contact;

    SA1Surface surface = world->query_surface(world->context,
                                              position.x, position.z);
    if (!surface.valid) return contact;

    contact.point = (SA1Vec3){position.x, surface.ground_y, position.z};
    contact.normal = normalize3(surface.normal);
    SA1Vec3 delta = {position.x - contact.point.x,
                     position.y - contact.point.y,
                     position.z - contact.point.z};
    contact.signed_distance = dot3(delta, contact.normal);
    contact.distance = length3(delta);
    contact.surface_id = surface.surface_id;

    SA1Vec3 probe = normalize3(probe_direction);
    SA1Vec3 toward_surface = {-contact.normal.x,
                              -contact.normal.y,
                              -contact.normal.z};
    float alignment = dot3(probe, toward_surface);
    if (contact.distance <= max_distance && alignment > 0.15f)
        contact.valid = true;
    return contact;
}
