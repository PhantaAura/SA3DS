#ifndef SA3DS_MOVEMENT_OBJECTS_H
#define SA3DS_MOVEMENT_OBJECTS_H
#include <stddef.h>
#include <stdint.h>
#include "../sa1/physics/player_state.h"
/* Borrowed package bytes must outlive this view. No actor allocation/spawn. */
typedef struct {const uint8_t *rows,*specs;uint32_t count;} SA3DSMovementObjects;
typedef struct {
    uint32_t act,source_row;
    uint16_t type;
    int16_t angles[3];
    SA1Vec3 position;
    float parameters[3];
} SA3DSMovementPlacement;
typedef struct {
    float strength;
    int16_t control_timer;
    uint8_t command,sonic_action;
    uint16_t animation_request;
} SA3DSSpringCommand;
int sa3ds_movement_objects_open(SA3DSMovementObjects *out,const void *package,size_t size);
int sa3ds_movement_objects_placement(const SA3DSMovementObjects *objects,uint32_t index,SA3DSMovementPlacement *out);
/* Decode the spring's final command only. Source hit arbitration, launch
 * placement, angle/frame conversion and the action9/10 movement are separate. */
int sa3ds_movement_spring_command(const SA3DSMovementPlacement *placement,SA3DSSpringCommand *out);
/* Original O JPanel slot table. A missing next slot selects slot zero. */
#define SA3DS_NUMBERED_PANEL_SLOTS 10
typedef struct {
    SA3DSMovementPlacement slots[SA3DS_NUMBERED_PANEL_SLOTS];
    uint16_t present;
} SA3DSNumberedPanels;
int sa3ds_numbered_panels_open(SA3DSNumberedPanels *out,const SA3DSMovementObjects *objects,unsigned act);
int sa3ds_numbered_panel_next(const SA3DSNumberedPanels *panels,unsigned from);
/* 4B8910 translational step. The caller supplies source-rotated target offsets.
 * Returns 1 for travel, 2 for arrival, 0 for invalid input. */
int sa3ds_numbered_panel_step(SA1Vec3 task,SA1Vec3 velocity,SA1Vec3 aim,SA1Vec3 landing,
    SA1Vec3 *next_task,SA1Vec3 *next_velocity);
#endif
