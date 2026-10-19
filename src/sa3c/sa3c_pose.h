#ifndef SA3DS_SA3C_POSE_H
#define SA3DS_SA3C_POSE_H

#include <stddef.h>
#include <stdint.h>

#include "sa3c_character.h"

typedef struct { float x, y, z; } SA3CVec3;
typedef struct {
    SA3CVec3 position;
    SA3CVec3 rotation; /* radians; source keys originate as Ninja BAMS */
    SA3CVec3 scale;
} SA3CTransform;

typedef struct {
    /* Row-major storage matching SA Tools' M11..M44 naming. */
    float m[16];
} SA3CMatrix;

typedef enum {
    SA3C_POSE_OK = 0,
    SA3C_POSE_ERR_ARGUMENT,
    SA3C_POSE_ERR_RANGE,
    SA3C_POSE_ERR_EMPTY_ACTION,
    SA3C_POSE_ERR_UNSUPPORTED_INTERPOLATION,
    SA3C_POSE_ERR_HIERARCHY
} SA3CPoseResult;

/* Ninja/SADX object evaluation flags used by the PC2004 source. */
enum {
    SA3C_EVAL_UNIT_POS   = 0x0001u,
    SA3C_EVAL_UNIT_ANG   = 0x0002u,
    SA3C_EVAL_UNIT_SCL   = 0x0004u,
    SA3C_EVAL_HIDE       = 0x0008u,
    SA3C_EVAL_BREAK      = 0x0010u,
    SA3C_EVAL_ZXY_ANG    = 0x0020u,
    SA3C_EVAL_SKIP       = 0x0040u,
    SA3C_EVAL_SHAPE_SKIP = 0x0080u,
    SA3C_EVAL_CLIP       = 0x0100u,
    SA3C_EVAL_MODIFIER   = 0x0200u
};

void sa3c_matrix_identity(SA3CMatrix *out);

/* Load bind transforms for every node in the package. */
SA3CPoseResult sa3c_pose_bind(
    const SA3CCharacter *c,
    SA3CTransform *local,
    size_t local_count);

/*
 * Sample one source ACTION into caller-owned local transforms.
 *
 * - Starts from bind transforms.
 * - Sonic's exact PC2004 motions use linear Ninja interpolation.
 * - Rotation interpolation follows SA Tools' raw-BAMS linear + away-from-zero
 *   quantization semantics before returning radians.
 * - No allocation and no gameplay/physics state access.
 */
SA3CPoseResult sa3c_pose_sample_action(
    const SA3CCharacter *c,
    uint16_t action_index,
    float frame,
    int loop,
    SA3CTransform *local,
    size_t local_count,
    uint16_t *out_root_node);

/* PC2004 PlayAnimation transition action (0x0043F2A0): sample the retained
 * source pose, sample the destination at frame zero, then interpolate local
 * position/scale linearly and rotation along the shortest signed-BAMS path. */
SA3CPoseResult sa3c_pose_sample_transition(
    const SA3CCharacter *c,
    uint16_t source_action_index,
    float source_frame,
    uint16_t destination_action_index,
    float weight,
    SA3CTransform *local,
    SA3CTransform *scratch,
    size_t local_count,
    uint16_t *out_root_node);

/* Build world matrices using Ninja object transform/eval semantics. */
SA3CPoseResult sa3c_pose_build_world(
    const SA3CCharacter *c,
    uint16_t root_node,
    const SA3CTransform *local,
    size_t local_count,
    SA3CMatrix *world,
    size_t world_count);

const char *sa3c_pose_result_name(SA3CPoseResult result);

#endif
