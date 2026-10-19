#ifndef SA1_PHYSICS_GROUND_H
#define SA1_PHYSICS_GROUND_H

#include <stdbool.h>
#include <stdint.h>
#include "player_state.h"

typedef struct {
    float ground_y;
    float grad_x;
    float grad_z;
    SA1Vec3 normal;
    float slope_radians;
    int32_t surface_id;
    bool valid;
} SA1Surface;

typedef struct {
    SA1Vec3 point;
    SA1Vec3 normal;
    float signed_distance;
    float distance;
    int32_t surface_id;
    uint32_t surface_flags;
    bool valid;
} SA1Contact;

typedef SA1Surface (*SA1SurfaceQuery)(void *context, float x, float z);
typedef SA1Contact (*SA1ContactQuery)(void *context, SA1Vec3 position,
                                      SA1Vec3 probe_direction,
                                      float max_distance);

typedef struct {
    SA1SurfaceQuery query_surface;
    SA1ContactQuery query_contact;
    void *context;
    SA1Vec3 reset_position;
    float reset_min_x;
    float reset_max_x;
    float reset_min_y;
    float reset_min_z;
    float reset_max_z;
} SA1PhysicsWorld;

SA1Vec3 sa1_surface_normal(float grad_x, float grad_z);
float sa1_surface_slope_radians(SA1Vec3 normal);

/* General support query used by 0.0.8.7. If a world has no arbitrary-contact
 * callback, this adapts the older heightfield query so existing stages/tests
 * stay source compatible. */
SA1Contact sa1_world_query_contact(const SA1PhysicsWorld *world,
                                   SA1Vec3 position,
                                   SA1Vec3 probe_direction,
                                   float max_distance);

#endif
