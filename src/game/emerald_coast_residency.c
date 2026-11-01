#include "emerald_coast_residency.h"
#include "../sa1/physics/movement.h"
#include <math.h>
int sa3ds_ec_residency_needs_recenter(const SA3DSECCollision*s,SA1Vec3 p){
    return s && isfinite(p.x)&&isfinite(p.z) &&
        (fabsf(p.x-s->center_x)>SA3DS_EC_REGION_RELOAD_MARGIN ||
         fabsf(p.z-s->center_z)>SA3DS_EC_REGION_RELOAD_MARGIN);
}
int sa3ds_ec_residency_recenter(SA3DSECCollision*s,const void*data,size_t size,
    float x,float z,SA1PlayerState*p){
    if(!s||!p)return 0;
    SA3DSECCollision next;
    if(!sa3ds_ec_collision_open_region(&next,data,size,s->act_index,x,z))return 0;
    uint32_t instance=UINT32_MAX,triangle=UINT32_MAX;
    if(p->surface_id>=0&&(uint32_t)p->surface_id<s->triangle_count){
        instance=s->triangles[p->surface_id].source_instance;
        triangle=s->triangles[p->surface_id].source_triangle;
    }
    next.world.reset_position=s->world.reset_position;
    next.world.reset_min_x=s->world.reset_min_x;next.world.reset_max_x=s->world.reset_max_x;
    next.world.reset_min_y=s->world.reset_min_y;
    next.world.reset_min_z=s->world.reset_min_z;next.world.reset_max_z=s->world.reset_max_z;
    next.reference_y=p->pos.y;
    next.queries=s->queries;next.triangle_tests=s->triangle_tests;next.wall_hits=s->wall_hits;
    int32_t surface=-1;
    for(uint32_t i=0;i<next.triangle_count;i++)
        if(next.triangles[i].source_instance==instance&&next.triangles[i].source_triangle==triangle){surface=(int32_t)i;break;}
    sa3ds_ec_collision_destroy(s);*s=next;
    s->world.context=s; /* The temporary loader's stack address must not escape. */
    p->surface_id=surface;
    return 1;
}
void sa3ds_ec_residency_start(SA3DSECCollision*s,const SA3DSECStart*start,SA1PlayerState*p){
    if(!s||!start||!p)return;
    s->world.reset_position=start->task_origin;
    s->world.reset_position.y+=SA3DS_EC_CONTROLLER_SUPPORT;
    sa1_player_reset(p,&s->world);
    p->yaw=sa3ds_ec_route_player_yaw(start->task_yaw);
    p->surface_forward=(SA1Vec3){sinf(p->yaw),0,cosf(p->yaw)};
    p->surface_right=(SA1Vec3){cosf(p->yaw),0,-sinf(p->yaw)};
    s->reference_y=p->pos.y;
}
