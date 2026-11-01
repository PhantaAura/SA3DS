#ifndef SA3DS_RING_RUNTIME_H
#define SA3DS_RING_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
#include "../sa1/physics/player_state.h"
#define SA3DS_RING_MAX_ROWS 2046u /* two original <=1023-slot act banks */
typedef struct {
    uint32_t act,source_row;
    uint16_t type;
    int16_t angles[3];
    SA1Vec3 position;
    float parameters[3];
    int32_t group; /* -1 direct SET ring, otherwise source parent group */
    uint8_t child_index;
} SA3DSRingPlacement;
typedef struct {
    uint8_t action,collected,pending_hit,distance_unloaded,consumed,pickup_normal_draw;
    uint16_t yaw;
    float pickup_age; /* third source particle age; art remains pending */
} SA3DSRingState;
typedef struct {
    SA3DSRingPlacement placement;
    uint32_t first_child,child_count;
    uint8_t action,journal,skip_children;
} SA3DSRingGroup;
typedef struct {
    const uint8_t *rows;
    uint32_t count,direct_count,act;
    SA1Vec3 collision_center;
    float radius,load_squared,unload_squared;
    uint16_t turn;
    unsigned clip_level;
    SA3DSRingState *states;
    SA3DSRingPlacement *actors;
    SA3DSRingGroup *groups;
    uint32_t group_count;
    const uint8_t *quarter_lut;
    float group_load_squared,group_unload_squared,child_unload_squared;
    int16_t rings;
    uint32_t collected_events,group_collected_events,extra_lives;
} SA3DSRings;
int sa3ds_rings_open(SA3DSRings *out,const void *package,size_t size);
void sa3ds_rings_destroy(SA3DSRings *rings);
int sa3ds_rings_placement(const SA3DSRings *rings,uint32_t index,SA3DSRingPlacement *out);
void sa3ds_rings_reset_act(SA3DSRings *rings,uint32_t act);
/* One ordinary gameplay tick. Caller provides the physical-frame primary Sonic
 * sphere and source debug/status eligibility. Cliplevel is the source graphics
 * clipping setting0..2 (default1), not a gameplay difficulty. */
uint32_t sa3ds_rings_tick(SA3DSRings *rings,SA1Vec3 task_origin,
    SA1Vec3 sphere_center,float sphere_radius,int debug_mode,int collect_eligible,
    unsigned clip_level);
uint32_t sa3ds_rings_tick_presented(SA3DSRings *rings,SA1Vec3 task_origin,
    SA1Vec3 sphere_center,float sphere_radius,int debug_mode,int collect_eligible,
    unsigned clip_level,int presentation_eligible);
/* Presentation-only normal-ring pulse on the pickup update. */
int sa3ds_rings_draw_normal(SA3DSRings *rings,uint32_t index);
/* Signed16 source counter, without an invented999 clamp. Returns100-ring awards. */
unsigned sa3ds_rings_add(SA3DSRings *rings,int16_t amount);
#endif
