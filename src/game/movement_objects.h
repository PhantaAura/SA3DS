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
#endif
