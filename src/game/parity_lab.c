#include "parity_lab.h"

#include <math.h>

#include "../sa1/physics/movement.h"

#define LAB_HALF_WIDTH 12.0f
#define LAB_MARKER_SPACING 10.0f

/* Adjacent ranges deliberately avoid seams. The course stays 24 units wide. */
static const SA3DSParityZone kZones[SA3DS_PARITY_ZONE_COUNT] = {
    {SA3DS_PARITY_ZONE_CALIBRATION,    "CALIBRATION",           -40.0f,  20.0f, -30.0f, 200},
    {SA3DS_PARITY_ZONE_ACCELERATION,   "ACCELERATION",           20.0f, 100.0f,  24.0f, 201},
    {SA3DS_PARITY_ZONE_BRAKE_COAST,    "BRAKE / COAST",        100.0f, 150.0f, 104.0f, 202},
    {SA3DS_PARITY_ZONE_TURN,           "TURN",                 150.0f, 205.0f, 154.0f, 203},
    {SA3DS_PARITY_ZONE_JUMP,           "JUMP",                 205.0f, 260.0f, 209.0f, 204},
    {SA3DS_PARITY_ZONE_ROLL_SPINDASH,  "ROLL / SPINDASH",      260.0f, 315.0f, 264.0f, 205},
    {SA3DS_PARITY_ZONE_SLOPE_UP,       "SLOPE UP",             315.0f, 355.0f, 319.0f, 206},
    {SA3DS_PARITY_ZONE_PLATEAU,        "PLATEAU",              355.0f, 385.0f, 359.0f, 207},
    {SA3DS_PARITY_ZONE_SLOPE_DOWN,     "SLOPE DOWN",           385.0f, 425.0f, 389.0f, 208},
    {SA3DS_PARITY_ZONE_AIR_LANDING,    "AIR CONTROL / LANDING",425.0f, 500.0f, 429.0f, 209},
};

static SA1PhysicsWorld gWorld = {
    .query_surface = sa3ds_parity_lab_surface,
    .query_contact = 0,
    .context = 0,
    .reset_position = {0.0f, 1.0f, -30.0f},
    .reset_min_x = -28.0f,
    .reset_max_x = 28.0f,
    .reset_min_y = -40.0f,
    .reset_min_z = -60.0f,
    .reset_max_z = 520.0f
};

static int in_course(float x, float z) {
    return fabsf(x) <= LAB_HALF_WIDTH && z >= kZones[0].z_start &&
           z <= kZones[SA3DS_PARITY_ZONE_COUNT - 1].z_end;
}

const SA3DSParityZone *sa3ds_parity_lab_zones(size_t *count_out) {
    if (count_out) *count_out = SA3DS_PARITY_ZONE_COUNT;
    return kZones;
}

const SA3DSParityZone *sa3ds_parity_lab_zone_by_id(SA3DSParityZoneId id) {
    if ((unsigned)id >= (unsigned)SA3DS_PARITY_ZONE_COUNT) return 0;
    return &kZones[(unsigned)id];
}

const SA3DSParityZone *sa3ds_parity_lab_zone_for_z(float z) {
    for (size_t i = 0; i < SA3DS_PARITY_ZONE_COUNT; ++i) {
        if (z >= kZones[i].z_start &&
            (z < kZones[i].z_end || i + 1u == SA3DS_PARITY_ZONE_COUNT))
            return &kZones[i];
    }
    return 0;
}

SA1Surface sa3ds_parity_lab_surface(void *context, float x, float z) {
    (void)context;
    SA1Surface s = {0};
    if (!in_course(x, z)) return s;

    const SA3DSParityZone *zone = sa3ds_parity_lab_zone_for_z(z);
    if (!zone) return s;
    s.surface_id = zone->surface_id;

    if (zone->id == SA3DS_PARITY_ZONE_SLOPE_UP) {
        const float rise = 8.0f;
        const float run = zone->z_end - zone->z_start;
        s.grad_z = rise / run;
        s.ground_y = (z - zone->z_start) * s.grad_z;
    } else if (zone->id == SA3DS_PARITY_ZONE_PLATEAU) {
        s.ground_y = 8.0f;
    } else if (zone->id == SA3DS_PARITY_ZONE_SLOPE_DOWN) {
        const float fall = -8.0f;
        const float run = zone->z_end - zone->z_start;
        s.grad_z = fall / run;
        s.ground_y = 8.0f + (z - zone->z_start) * s.grad_z;
    } else {
        s.ground_y = 0.0f;
    }

    s.normal = sa1_surface_normal(s.grad_x, s.grad_z);
    s.slope_radians = sa1_surface_slope_radians(s.normal);
    s.valid = true;
    return s;
}

const SA1PhysicsWorld *sa3ds_parity_lab_world(void) {
    return &gWorld;
}

int sa3ds_parity_lab_reset_player(SA1PlayerState *player, SA3DSParityZoneId id) {
    if (!player) return 0;
    const SA3DSParityZone *zone = sa3ds_parity_lab_zone_by_id(id);
    if (!zone) return 0;

    SA1Surface s = sa3ds_parity_lab_surface(0, 0.0f, zone->reset_z);
    if (!s.valid) return 0;

    /* Crucial 0G behavior: the same selected-zone position is used by the
       physics layer if a later step falls or runs out of bounds. */
    gWorld.reset_position.x = 0.0f;
    gWorld.reset_position.y = s.ground_y + 1.0f;
    gWorld.reset_position.z = zone->reset_z;

    sa1_player_reset(player, &gWorld);
    player->ground_normal = s.normal;
    player->ground_slope_radians = s.slope_radians;
    player->surface_id = s.surface_id;
    player->surface_forward = (SA1Vec3){0.0f, s.grad_z, 1.0f};
    float len = sqrtf(player->surface_forward.y * player->surface_forward.y + 1.0f);
    player->surface_forward.y /= len;
    player->surface_forward.z /= len;
    player->surface_right = (SA1Vec3){1.0f, 0.0f, 0.0f};
    player->support_distance = 1.0f;
    player->yaw = 0.0f;
    return 1;
}

float sa3ds_parity_lab_half_width(void) { return LAB_HALF_WIDTH; }
float sa3ds_parity_lab_marker_spacing(void) { return LAB_MARKER_SPACING; }
