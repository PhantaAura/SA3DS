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
typedef struct {
    uint32_t remaining[3], total;
    unsigned phase, timer, visible;
    uint32_t final_score;
} SA3DSClearScore;
typedef struct {
    uint32_t score, time_ticks;
    uint16_t rings;
    uint8_t emblems;
} SA3DSClearRecord;
/* PC2004 42A680 case 2 compares each record independently. */
int sa3ds_clear_record_update(SA3DSClearRecord *record, uint32_t score,
    const uint8_t time[3], uint16_t rings);
/* Highest unearned mission has priority, even when its condition fails.
 * Returns the attempted mission (A=0, B=1, C=2), or -1 when all are earned. */
int sa3ds_clear_emblem(SA3DSClearRecord *record,const uint8_t time[3],
    uint16_t rings,int *awarded);
/* 4283C0 ordinary results sequence, with the external award task's busy flag. */
typedef struct {unsigned phase,ticks;} SA3DSClearExit;
enum {SA3DS_CLEAR_AWARD=1,SA3DS_CLEAR_LEAVE=2};
unsigned sa3ds_clear_exit_tick(SA3DSClearExit *exit,int mission,int awarded,int award_busy);
/* Ordinary Sonic Emerald Coast branch of 42A680 / 4266C0. */
void sa3ds_clear_score_begin(SA3DSClearScore *clear,uint32_t score,unsigned rings,
    unsigned minutes,unsigned seconds);
enum {
    SA3DS_CLEAR_TIME=1u, SA3DS_CLEAR_RINGS=2u, SA3DS_CLEAR_SCORE_BONUS=4u,
    SA3DS_CLEAR_TIME_BONUS=8u, SA3DS_CLEAR_RING_BONUS=16u, SA3DS_CLEAR_TOTAL=32u,
    SA3DS_CLEAR_REVEAL_SOUND=5, SA3DS_CLEAR_TALLY_SOUND=4
};
/* Returns the SE request for this update, or zero. Phase 12 hands off to
 * the record/emblem sequence. */
unsigned sa3ds_clear_score_tick(SA3DSClearScore *clear,int advance);
#endif
