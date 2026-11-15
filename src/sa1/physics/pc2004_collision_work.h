#ifndef SA1_PHYSICS_PC2004_COLLISION_WORK_H
#define SA1_PHYSICS_PC2004_COLLISION_WORK_H

#include "ground.h"
#include "player_state.h"
#include "steam_player_parameters.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Portable layout mirror of the 2004-PC player collision/support work block
 * reached through playerwk+0x64.  The 0x84-byte size and offsets are verified
 * against the exact supported 2004 executable; field names are cross-checked
 * against the public SADX symbol headers (X-Hax SADXStructsNew.h `csts`).
 *
 * This is metadata/state, not a copy of the original collision solver.
 */
typedef struct {
    float radius;                 /* 0x00 */
    SA1Vec3 pos;                  /* 0x04 */
    SA1Vec3 spd;                  /* 0x10 */
    SA1Vec3 tnorm;                /* 0x1C */
    uint16_t find_count;          /* 0x28 */
    uint16_t selected_nmb;        /* 0x2A */
    float yt;                     /* 0x2C */
    float yb;                     /* 0x30 */
    int32_t angx;                 /* 0x34 */
    int32_t angz;                 /* 0x38 */
    SA1Vec3 normal;               /* 0x3C */
    SA1Vec3 normal2;              /* 0x48 */
    SA1Vec3 onpoly;               /* 0x54 */
    SA1Vec3 pshbk;                /* 0x60 */
    SA1Vec3 anaspdh;              /* 0x6C */
    SA1Vec3 anaspdv;              /* 0x78 */
} SA1PC2004CollisionWork;

_Static_assert(sizeof(SA1PC2004CollisionWork) == 0x84,
               "2004 csts layout must be exactly 0x84 bytes");
_Static_assert(offsetof(SA1PC2004CollisionWork, tnorm) == 0x1C, "csts.tnorm offset");
_Static_assert(offsetof(SA1PC2004CollisionWork, find_count) == 0x28, "csts.find_count offset");
_Static_assert(offsetof(SA1PC2004CollisionWork, selected_nmb) == 0x2A, "csts.selected_nmb offset");
_Static_assert(offsetof(SA1PC2004CollisionWork, angx) == 0x34, "csts.angx offset");
_Static_assert(offsetof(SA1PC2004CollisionWork, normal) == 0x3C, "csts.normal offset");
_Static_assert(offsetof(SA1PC2004CollisionWork, normal2) == 0x48, "csts.normal2 offset");
_Static_assert(offsetof(SA1PC2004CollisionWork, onpoly) == 0x54, "csts.onpoly offset");
_Static_assert(offsetof(SA1PC2004CollisionWork, pshbk) == 0x60, "csts.pshbk offset");
_Static_assert(offsetof(SA1PC2004CollisionWork, anaspdh) == 0x6C, "csts.anaspdh offset");
_Static_assert(offsetof(SA1PC2004CollisionWork, anaspdv) == 0x78, "csts.anaspdv offset");

/* Evidence-safe initialization reproduced from the normal 2004 PSetPosition
 * helper path: zero work, radius=height*0.5, tnorm=(0,1,0), copy task X/Z
 * angles, and seed position/speed. */
void sa1_pc2004_collision_work_init(
    SA1PC2004CollisionWork *work,
    const SA1SteamPlayerParameters *p,
    SA1Vec3 pos,
    SA1Vec3 speed,
    int32_t angx,
    int32_t angz);

/* Movement-Lab diagnostic mirror only.  This records the contact selected by
 * our synthetic world in the equivalent named work fields. It is NOT claimed
 * to reproduce the retail polygon candidate solver or its internal response
 * vectors. */
void sa1_pc2004_collision_work_mirror_static_contact(
    SA1PC2004CollisionWork *work,
    const SA1Contact *contact,
    uint16_t selected_index);

/* Ground-side candidate gate shared by 43CCE0, 43D180 and 43D360.
 * Normals are unit length. Gravity is supplied in world space. The
 * object flag 0x4000 exception applies to the 43D360 response only. */
int sa1_pc2004_collision_ground_side(SA1Vec3 up, SA1Vec3 normal, SA1Vec3 gravity,
                                     int force_ground_side);

/* Position in the ordinary 449EE5 support response, before the task-position
 * write gate. corrected_center is the sweep's average radius-corrected point,
 * tangent_step comes from the selected face, and plane_normal is nonzero. */
SA1Vec3 sa1_pc2004_collision_support_position(SA1Vec3 corrected_center,
    SA1Vec3 tangent_step, SA1Vec3 plane_point, SA1Vec3 plane_normal);

/* Candidate from the first pass. The distance uses the radius-corrected
 * center returned by 453820, measured from the original sweep center. */
typedef struct {
    SA1Vec3 normal;
    float corrected_distance;
} SA1PC2004CollisionCandidate;

/* Return the clipped length for the 43CCE0/43D180 prepass. A strictly
 * shorter result promotes that object's entry in the second pass. */
float sa1_pc2004_collision_prepass_length(
    SA1Vec3 up, SA1Vec3 gravity, SA1Vec3 motion, int supported,
    const SA1PC2004CollisionCandidate *candidates, size_t count);

/* Produce input indices in second-pass order. promoted and previous may
 * be SIZE_MAX. Input is filtered and contains one entry per collision
 * identity; order has room for count indices. */
void sa1_pc2004_collision_response_order(size_t count, size_t promoted,
    size_t previous, int moving_support, size_t *order);

/* Ordinary supported Sonic at or above dash speed. 444640 starts the
 * confirmation sweep at task + Up * (4 + probe_offset) + pushback,
 * where pushback is the averaged corrected center minus the first center.
 * The confirmation radius is 5.1 in this specialization. */
SA1Vec3 sa1_pc2004_fast_support_probe_center(SA1Vec3 task, SA1Vec3 up,
    SA1Vec3 pushback, float local_vertical_speed, float pos_error);

/* Final speed gate after geometric confirmation in 449EE5 position response.
 * low_ground_bits is the current player-work status masked to bits 0 and 1. */
int sa1_pc2004_collision_support_speed(SA1Vec3 travel, SA1Vec3 normal,
    float run_speed, float local_vertical_speed, unsigned low_ground_bits,
    uint32_t flags);

#endif
