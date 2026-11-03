#ifndef SA3DS_PLAYER_DEATH_H
#define SA3DS_PLAYER_DEATH_H
#include "player_damage.h"
typedef struct {uint32_t counter;uint8_t active;} SA3DSPlayerDeathTask;
typedef struct {uint8_t stage_request,parent_status4,audio_cleanup,destroy;} SA3DSPlayerDeathTaskResult;
/*440C80: call in list0 BEFORE the next Sonic update. A newly allocated head
 * is not visited by the snapshot-next task walker in its creation update.*/
int sa3ds_player_death_task_tick(SA3DSPlayerDeathTask *task,int special_mode,int has_parent,
    SA3DSPlayerDeathTaskResult *out);
/*Ordinary495FA0 command50. Does not erase source status1000/command provenance.
 * Audio return uses the same channel convention as player_damage.*/
int sa3ds_player_death_command(SA3DSPlayerDamageState *state,int alternate,
    uint16_t *sound_id,uint8_t *sound_channel);
/*441F80 uses a pre-incremented UINT16 counter, unlike death440C80.
 * Apply in list0 before Sonic; nullable flags means player work is absent.*/
typedef struct {uint16_t counter;uint8_t active;} SA3DSPlayerProtectionTask;
int sa3ds_player_protection_tick(SA3DSPlayerProtectionTask *task,
    uint16_t *player_flags6,uint8_t *destroy);
#endif
