#ifndef SA3DS_EMERALD_COAST_ROUTE_H
#define SA3DS_EMERALD_COAST_ROUTE_H
#include "../sa1/physics/player_state.h"
#include <stddef.h>
#define SA3DS_EC_ROUTE_MAX_ACTS 8u
typedef struct { uint32_t act_index;SA1Vec3 task_origin;uint16_t task_yaw; } SA3DSECStart;
typedef struct { uint32_t from_act,to_act;SA1Vec3 center;float radius; } SA3DSECActEdge;
typedef struct {
    uint32_t act_index,source_row;
    uint16_t type;
    SA1Vec3 center;
    float radius,load_distance_squared;
} SA3DSECGoal;
typedef struct {
    uint32_t start_count,edge_count;
    SA3DSECStart starts[SA3DS_EC_ROUTE_MAX_ACTS];
    SA3DSECActEdge edges[SA3DS_EC_ROUTE_MAX_ACTS];
    SA3DSECGoal goal;
    uint8_t has_goal;
} SA3DSECRoute;
int sa3ds_ec_route_open(SA3DSECRoute *out,const void *package,size_t size);
const SA3DSECStart *sa3ds_ec_route_start(const SA3DSECRoute *route,uint32_t act_index);
/* Input is the original TASK origin, not the controller support-center. */
int sa3ds_ec_route_next(const SA3DSECRoute *route,uint32_t act_index,
    SA1Vec3 task_origin,uint32_t *next_act);
/* Movement rotates original local+X through -taskY. Port yaw is measured
 * from world+Z toward +X, hence yaw=PI/2-taskY. No model/terrain rescaling. */
float sa3ds_ec_route_player_yaw(uint16_t task_yaw);
#endif
