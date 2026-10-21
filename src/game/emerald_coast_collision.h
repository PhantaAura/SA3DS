#ifndef SA3DS_EMERALD_COAST_COLLISION_H
#define SA3DS_EMERALD_COAST_COLLISION_H

#include "../sa1/physics/ground.h"
#include "sa3c_pose.h"
#include <stddef.h>

/* Bounded opening-area integration, not the retail polygon solver. The
 * existing controller retains its 1-unit support offset. */
#define SA3DS_EC_GRID_SIDE 16u
#define SA3DS_EC_CONTROLLER_SUPPORT 1.0f
/* Provisional beach support on imported act-0 instance 74, triangle 49.
 * Not a recovered retail StartPosition. */
#define SA3DS_EC_SPAWN_X 0.0f
#define SA3DS_EC_SPAWN_Z (-50.0f)
typedef struct {
    SA1Vec3 a,b,c,normal;
    float min_x,max_x,min_z,max_z;
    uint32_t source_instance,source_triangle,flags;
} SA3DSECTriangle;
typedef struct {
    SA1PhysicsWorld world;
    SA3DSECTriangle *triangles;
    uint32_t triangle_count,instance_count;
    uint32_t cells[SA3DS_EC_GRID_SIDE*SA3DS_EC_GRID_SIDE+1u];
    uint32_t *references,*visited;
    uint32_t reference_count,query_serial,queries,triangle_tests,wall_hits;
    float reference_y;
} SA3DSECCollision;

int sa3ds_ec_collision_open(SA3DSECCollision *out,const void *package,size_t size);
void sa3ds_ec_collision_destroy(SA3DSECCollision *stage);
SA1Surface sa3ds_ec_collision_surface(void *context,float x,float z);
SA1Contact sa3ds_ec_collision_contact(void *context,SA1Vec3 position,
    SA1Vec3 probe,float range);
/* Select a support triangle near the stated height; never invent retail spawn. */
int sa3ds_ec_collision_spawn(SA3DSECCollision *stage,float x,float z,float hint_y);
void sa3ds_ec_collision_resolve_walls(SA3DSECCollision *stage,
    SA1Vec3 previous,SA1PlayerState *player);
/* Bridge the inherited controller support-center convention to PC2004's task
 * foot origin and Sonic render center_height. No mesh-dependent foot lift. */
SA1Vec3 sa3ds_ec_task_origin(const SA1PlayerState *player);
void sa3ds_ec_model_transform(const SA1PlayerState *player,SA3CMatrix *out);

#endif
