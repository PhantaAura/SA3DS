#ifndef SA3DS_SARU_POSE_H
#define SA3DS_SARU_POSE_H
#include "saru.h"
#include "../sa3c/sa3c_pose.h"
/* Exact E SARU profile only: six shared-root ACTIONs,38 nodes, linear type3.
 *406EC0/406C40 wraps each motion independently;406140 and406680 use raw
 *integer Euler angles, not Sonic43F2A0's synthetic shortest-angle transition.
 * Caller owns38 transforms. No animation-clock advancement or allocation. */
SA3CPoseResult sa3ds_saru_pose_sample(const SA3CCharacter *character,
    const SA3DSSaruAnimation *animation,SA3CTransform local[38],uint16_t *root);
#endif
