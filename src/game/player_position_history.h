#ifndef SA3DS_PLAYER_POSITION_HISTORY_H
#define SA3DS_PLAYER_POSITION_HISTORY_H
#include <stdint.h>
/*Position-only projection of one player's43B960/4419C0 ring. The original
 *also records task rotation; this service does not synthesize angle history.*/
typedef struct {float position[256][3];uint8_t cursor,present;} SA3DSPlayerPositionHistory;
/*43B920 clears player presence, not these history bytes/cursor. Static zero
 *initialization is the cold-start state; do not prefill on a stage reset.*/
void sa3ds_player_position_history_present(SA3DSPlayerPositionHistory *history,int present);
int sa3ds_player_position_history_record(SA3DSPlayerPositionHistory *history,const float position[3]);
/*4419C0 returns0 without touching output if player is absent. A null output
 *is permitted; no current-position fallback or warm-up age clamp is applied.*/
int sa3ds_player_position_history_read(const SA3DSPlayerPositionHistory *history,uint8_t offset,float out[3]);
/*441A40 position-only setter. Does not advance cursor; null input is a no-op.*/
int sa3ds_player_position_history_write(SA3DSPlayerPositionHistory *history,uint8_t offset,const float position[3]);
#endif
