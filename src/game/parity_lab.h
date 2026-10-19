#ifndef SA3DS_GAME_PARITY_LAB_H
#define SA3DS_GAME_PARITY_LAB_H

#include <stddef.h>
#include <stdint.h>

#include "../sa1/physics/ground.h"
#include "../sa1/physics/player_state.h"

typedef enum {
    SA3DS_PARITY_ZONE_CALIBRATION = 0,
    SA3DS_PARITY_ZONE_ACCELERATION,
    SA3DS_PARITY_ZONE_BRAKE_COAST,
    SA3DS_PARITY_ZONE_TURN,
    SA3DS_PARITY_ZONE_JUMP,
    SA3DS_PARITY_ZONE_ROLL_SPINDASH,
    SA3DS_PARITY_ZONE_SLOPE_UP,
    SA3DS_PARITY_ZONE_PLATEAU,
    SA3DS_PARITY_ZONE_SLOPE_DOWN,
    SA3DS_PARITY_ZONE_AIR_LANDING,
    SA3DS_PARITY_ZONE_COUNT
} SA3DSParityZoneId;

typedef struct {
    SA3DSParityZoneId id;
    const char *name;
    float z_start;
    float z_end;
    float reset_z;
    int32_t surface_id;
} SA3DSParityZone;

/* Developer-only original White Zone geometry. */
SA1Surface sa3ds_parity_lab_surface(void *context, float x, float z);
const SA1PhysicsWorld *sa3ds_parity_lab_world(void);
const SA3DSParityZone *sa3ds_parity_lab_zones(size_t *count_out);
const SA3DSParityZone *sa3ds_parity_lab_zone_for_z(float z);
const SA3DSParityZone *sa3ds_parity_lab_zone_by_id(SA3DSParityZoneId id);

/* Reset also arms the physics world's out-of-bounds fallback for this zone. */
int sa3ds_parity_lab_reset_player(SA1PlayerState *player, SA3DSParityZoneId id);

float sa3ds_parity_lab_half_width(void);
float sa3ds_parity_lab_marker_spacing(void);

#endif
