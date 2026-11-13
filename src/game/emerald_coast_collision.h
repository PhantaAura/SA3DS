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
    /* Static landtable triangles occupy [0,static_triangle_count); applied
     * DynamicCOL triangles follow. The set itself is caller-owned and carried
     * across residency rebuilds. */
    uint32_t static_triangle_count;
    const struct SA3DSECDynamicSet *dynamic;
} SA3DSECCollision;

/* 43B2E0 DynamicCOL projection: world-space triangles per owner task, copied
 * when added. Source source_instance is 0x80000000|owner, source_triangle the
 * owner-local index. Edge masks are derived within one owner's triangles. */
/* Emerald Coast act0 can contain four 120-triangle O JUMP bases and
 * 62 two-triangle pier decks. */
#define SA3DS_EC_DYNAMIC_MAX_TRIANGLES 640u
#define SA3DS_EC_DYNAMIC_MAX_OWNERS 128u
#define SA3DS_EC_DYNAMIC_SOURCE 0x80000000u
typedef struct {uint32_t owner,first,count;} SA3DSECDynamicOwner;
/* 419C90 top contact -> status0x20 -> 49A9B0 ground bit2 on the next update.
 * One-update object support: a yaw rectangle (already expanded by the foot
 * radius, 4199D0 corners) whose top plane is top-.01. Cleared by the caller
 * at each CCL phase, exactly like the source per-frame status clear. */
#define SA3DS_EC_OBJECT_SUPPORT_MAX 8u
/* normal: up axis of the hit object's rotX/Z, which49A9B0 copies to Sonic. */
typedef struct {float center_x,center_z,half_x,half_z,sin_yaw,cos_yaw,top_y;uint32_t owner;SA1Vec3 normal;} SA3DSECObjectSupport;
typedef struct SA3DSECDynamicSet {
    SA3DSECTriangle triangles[SA3DS_EC_DYNAMIC_MAX_TRIANGLES];
    SA3DSECDynamicOwner owners[SA3DS_EC_DYNAMIC_MAX_OWNERS];
    uint32_t triangle_count,owner_count,changes;
    SA3DSECObjectSupport supports[SA3DS_EC_OBJECT_SUPPORT_MAX];uint32_t support_count;
} SA3DSECDynamicSet;
void sa3ds_ec_dynamic_clear(SA3DSECDynamicSet *set);
/* Transform local points by the object's matrix; degenerate triangles skip.
 * An owner may be registered once; capacity exhaustion fails atomically. */
int sa3ds_ec_dynamic_add(SA3DSECDynamicSet *set,uint32_t owner,uint32_t flags,
    const float (*points)[3],uint32_t point_count,const uint16_t *indices,uint32_t index_count,
    const SA3CMatrix *transform);
/* 43B380: returns whether the owner had an entry. */
int sa3ds_ec_dynamic_remove(SA3DSECDynamicSet *set,uint32_t owner);
int sa3ds_ec_object_support_add(SA3DSECDynamicSet *set,const SA3DSECObjectSupport *support);
void sa3ds_ec_object_support_clear(SA3DSECDynamicSet *set);
/* Strict 4550A0-style test of an object support rectangle at (x,z). */
int sa3ds_ec_object_support_inside(const SA3DSECObjectSupport *support,float x,float z);
/* Rebuild the dynamic tail and spatial index from the attached set. A player
 * standing on a dynamic triangle keeps it by source identity. */
int sa3ds_ec_collision_apply_dynamic(SA3DSECCollision *stage,SA1PlayerState *player);

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
/* A valid region may be supported entirely by registered DynamicCOL. */
int sa3ds_ec_collision_open_archive_region_dynamic(SA3DSECCollision *out,
    const SA3DSECCollisionArchive *archive,uint32_t act_index,float center_x,float center_z,
    const SA3DSECDynamicSet *dynamic);
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
/* 4370C0/43A060 camera ray projection: resident triangles whose AABB meets the
 * swept sphere are handed, grouped per source instance, to the 439620 port
 * (sa3ds_pc2004_camera_ray). Water/no-floor flags 0x400002 are skipped like
 * the floor query (450C10's own filter is not decoded). At most 256
 * candidates; returns1 on a hit with start/delta rewritten. */
int sa3ds_ec_collision_camera_ray(SA3DSECCollision *stage,float start[3],float delta[3],float radius);
void sa3ds_ec_collision_resolve_walls(SA3DSECCollision *stage,
    SA1Vec3 previous,SA1PlayerState *player);
/* Bridge the inherited controller support-center convention to PC2004's task
 * foot origin and Sonic render center_height. No mesh-dependent foot lift. */
SA1Vec3 sa3ds_ec_task_origin(const SA1PlayerState *player);
void sa3ds_ec_model_transform(const SA1PlayerState *player,SA3CMatrix *out);

#endif
