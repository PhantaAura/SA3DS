#include "surface_frame.h"
#include <math.h>
#include <string.h>

float sa1_vec3_dot(SA1Vec3 a, SA1Vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

SA1Vec3 sa1_vec3_cross(SA1Vec3 a, SA1Vec3 b) {
    return (SA1Vec3){a.y*b.z - a.z*b.y,
                     a.z*b.x - a.x*b.z,
                     a.x*b.y - a.y*b.x};
}

float sa1_vec3_length(SA1Vec3 v) {
    return sqrtf(sa1_vec3_dot(v, v));
}

SA1Vec3 sa1_vec3_normalize(SA1Vec3 v) {
    float len = sa1_vec3_length(v);
    if (len <= 1e-8f) return (SA1Vec3){0.0f, 0.0f, 0.0f};
    return (SA1Vec3){v.x/len, v.y/len, v.z/len};
}

SA1Vec3 sa1_vec3_scale(SA1Vec3 v, float s) {
    return (SA1Vec3){v.x*s, v.y*s, v.z*s};
}

SA1Vec3 sa1_vec3_add(SA1Vec3 a, SA1Vec3 b) {
    return (SA1Vec3){a.x+b.x, a.y+b.y, a.z+b.z};
}

SA1Vec3 sa1_vec3_sub(SA1Vec3 a, SA1Vec3 b) {
    return (SA1Vec3){a.x-b.x, a.y-b.y, a.z-b.z};
}

SA1Vec3 sa1_project_on_plane(SA1Vec3 v, SA1Vec3 normal) {
    SA1Vec3 n = sa1_vec3_normalize(normal);
    return sa1_vec3_sub(v, sa1_vec3_scale(n, sa1_vec3_dot(v, n)));
}

SA1Vec3 sa1_rotate_around_axis(SA1Vec3 v, SA1Vec3 axis, float radians) {
    SA1Vec3 n = sa1_vec3_normalize(axis);
    float c = cosf(radians), s = sinf(radians);
    SA1Vec3 term0 = sa1_vec3_scale(v, c);
    SA1Vec3 term1 = sa1_vec3_scale(sa1_vec3_cross(n, v), s);
    SA1Vec3 term2 = sa1_vec3_scale(n, sa1_vec3_dot(n, v) * (1.0f - c));
    return sa1_vec3_add(sa1_vec3_add(term0, term1), term2);
}

SA1SurfaceFrame sa1_surface_frame_make(SA1Vec3 normal,
                                       SA1Vec3 preferred_forward) {
    SA1SurfaceFrame frame;
    memset(&frame, 0, sizeof(frame));
    frame.up = sa1_vec3_normalize(normal);
    if (sa1_vec3_length(frame.up) < 0.5f) return frame;

    frame.forward = sa1_project_on_plane(preferred_forward, frame.up);
    if (sa1_vec3_length(frame.forward) < 1e-5f) {
        SA1Vec3 fallback = fabsf(frame.up.y) < 0.9f ?
            (SA1Vec3){0.0f, 1.0f, 0.0f} :
            (SA1Vec3){0.0f, 0.0f, 1.0f};
        frame.forward = sa1_project_on_plane(fallback, frame.up);
    }
    frame.forward = sa1_vec3_normalize(frame.forward);
    frame.right = sa1_vec3_normalize(sa1_vec3_cross(frame.up, frame.forward));
    frame.forward = sa1_vec3_normalize(sa1_vec3_cross(frame.right, frame.up));
    frame.valid = sa1_vec3_length(frame.forward) > 0.5f &&
                  sa1_vec3_length(frame.right) > 0.5f;
    return frame;
}

float sa1_surface_tangent_speed(const SA1PlayerState *player) {
    if (!player) return 0.0f;
    SA1Vec3 tangent = sa1_project_on_plane(player->vel, player->ground_normal);
    return sa1_vec3_length(tangent);
}
