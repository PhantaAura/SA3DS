#ifndef SA3DS_PLAYER_DEATH_PLAYER_H
#define SA3DS_PLAYER_DEATH_PLAYER_H
#include "player_death.h"
/*Consume the command produced by an actual accepted zero-ring damage event.
 * Caller separately allocates/schedules440C80 and handles the audio request.*/
int sa3ds_player_death_player_command(SA1PlayerState *player,int alternate,
    uint16_t *sound_id,uint8_t *sound_channel);
#endif
