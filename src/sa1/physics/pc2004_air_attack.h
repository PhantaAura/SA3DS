#ifndef SA1_PC2004_AIR_ATTACK_H
#define SA1_PC2004_AIR_ATTACK_H
#include <stdint.h>
#include "pc2004_motion.h"
typedef struct {uint8_t action;uint16_t status,animation;int16_t timer,age;} SA1PC2004AirAttack;
typedef struct {SA1PC2004AirAttack next;int entered,write_forward,skip_jump_hold;float forward;} SA1PC2004AirAttackSelection;
/* Source action selector before494B80. Entered action14 starts timer-1/age0;
 * caller must run real initializer and same-tick movement, in that order. */
SA1PC2004AirAttackSelection sa1_pc2004_air_attack_select(
    SA1PC2004AirAttack current,int jump_pressed,int action_pressed,int first_hit_category);
/* Ordinary empty-target-list/441BF0==-1 specialization of action8->14.
 * Source CCL first other category is supplied separately (-1 means none).
 * special_player is player-work flag8000, not task status8000.
 * Commands, landing classification, target pursuit, effects and sound remain
 * caller-owned. No guessed target search or fall/action12 attack entry. */
SA1PC2004AirAttackSelection sa1_pc2004_air_attack_select_no_target(
    SA1PC2004AirAttack current,int jump_pressed,int action_pressed,int first_hit_category,int special_player);
/*492300 positive timer path, BEFORE44C270+443F50. Leaves normal/side intact.
 * Returns0 for target-tracking clocks, which require the actual target path. */
int sa1_pc2004_air_attack_decay(SA1PC2004AirAttack *state,SA1PC2004LocalVector *speed);
#endif
