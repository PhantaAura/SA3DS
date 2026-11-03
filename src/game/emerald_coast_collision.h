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
    /* Bits 0..2 identify boundary/sharp wall edges. Coplanar shared mesh
     * diagonals are excluded so finite-edge contact cannot snag on them. */
    uint8_t wall_edge_mask;
    /* Shared upward edges with continuous normals. Boundary ledges are never
     * marked, preserving genuine support departure. */
    uint8_t support_edge_mask;
} SA3DSECTriangle;
/* Borrowed immutable package view: prepare once after loading, then keep the
 * package bytes alive and unchanged until all regions using this view die. */
typedef struct {
    const void *package;
    size_t package_size;
    const uint8_t *collision;
    uint32_t act_count,model_count,instance_count,vertex_count,index_count;
    uint32_t act_offset,model_offset,instance_offset,vertex_offset,index_offset;
} SA3DSECCollisionArchive;
typedef struct {
    SA1PhysicsWorld world;
    SA3DSECCollisionArchive archive;
    SA3DSECTriangle *triangles;
    uint32_t triangle_count,instance_count;
    uint32_t cells[SA3DS_EC_GRID_SIDE*SA3DS_EC_GRID_SIDE+1u];
    uint32_t *references,*visited;
    uint32_t reference_count,query_serial,queries,triangle_tests,wall_hits;
    float reference_y;
    /* Grid cells are local to this bounded region; triangle/player positions
     * remain in the owner's original world units and coordinates. */
    uint32_t act_index;
    float center_x,center_z;
    float min_x,max_x,min_z,max_z;
} SA3DSECCollision;

/* Default opening region: act0, center(0,0), instance radius500 and grid
 * +/-512. Region selection does not spawn/reset or move the player. */
int sa3ds_ec_collision_open(SA3DSECCollision *out,const void *package,size_t size);
int sa3ds_ec_collision_open_region(SA3DSECCollision *out,
    const void *package,size_t size,uint32_t act_index,float center_x,float center_z);
/* The ordinary open functions validate fresh/untrusted bytes. Regions opened
 * from a prepared archive reuse that validation, never cache by address alone. */
int sa3ds_ec_collision_archive_open(SA3DSECCollisionArchive *out,
    const void *package,size_t size);
int sa3ds_ec_collision_open_archive_region(SA3DSECCollision *out,
    const SA3DSECCollisionArchive *archive,uint32_t act_index,
    float center_x,float center_z);
void sa3ds_ec_collision_destroy(SA3DSECCollision *stage);
/* Static resident-triangle adapter for49E920: highest upward floor no higher
 * than ceiling, ignoring land flags400002. Returns0 outside the guaranteed
 * resident radius (not a missing floor);1 includes source no-floor -1e6.
 * Polygon edge arithmetic/dynamic platforms are not retail-equivalent. */
int sa3ds_ec_collision_floor_below(SA3DSECCollision *stage,float x,float ceiling,float z,float *height);
/*Same selected static floor, also returns its upward normal (upright if absent).*/
int sa3ds_ec_collision_floor_below_frame(SA3DSECCollision *stage,float x,float ceiling,float z,float *height,SA1Vec3 *normal);
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
