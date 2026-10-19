#ifndef SA1_PHYSICS_SURFACE_FRAME_H
#define SA1_PHYSICS_SURFACE_FRAME_H

#include "player_state.h"

typedef struct {
    SA1Vec3 up;
    SA1Vec3 forward;
    SA1Vec3 right;
    int valid;
} SA1SurfaceFrame;

float sa1_vec3_dot(SA1Vec3 a, SA1Vec3 b);
SA1Vec3 sa1_vec3_cross(SA1Vec3 a, SA1Vec3 b);
float sa1_vec3_length(SA1Vec3 v);
SA1Vec3 sa1_vec3_normalize(SA1Vec3 v);
SA1Vec3 sa1_vec3_scale(SA1Vec3 v, float s);
SA1Vec3 sa1_vec3_add(SA1Vec3 a, SA1Vec3 b);
SA1Vec3 sa1_vec3_sub(SA1Vec3 a, SA1Vec3 b);
SA1Vec3 sa1_project_on_plane(SA1Vec3 v, SA1Vec3 normal);
SA1Vec3 sa1_rotate_around_axis(SA1Vec3 v, SA1Vec3 axis, float radians);
SA1SurfaceFrame sa1_surface_frame_make(SA1Vec3 normal,
                                       SA1Vec3 preferred_forward);
float sa1_surface_tangent_speed(const SA1PlayerState *player);

#endif
