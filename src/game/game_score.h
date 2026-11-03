#ifndef SA3DS_GAME_SCORE_H
#define SA3DS_GAME_SCORE_H
#include <stdint.h>
typedef struct {uint32_t total;int32_t chain_amount,chain_timer;} SA3DSGameScore;
/*425C70: first award starts chain amount0; later awards add with signed500
 * cap, reset timer120, and add only the original award to total. */
void sa3ds_game_score_add(SA3DSGameScore *score,int32_t award);
/*426070 active-timer branch: test OLD timer<0, then decrement, then clamp.
 * When the stage timer is disabled this branch does not run. */
void sa3ds_game_score_tick(SA3DSGameScore *score,int stage_timer_enabled);
#endif
