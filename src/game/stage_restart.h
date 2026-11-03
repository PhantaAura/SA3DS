#ifndef SA3DS_STAGE_RESTART_H
#define SA3DS_STAGE_RESTART_H
#include <stdint.h>
/*Ordinary action-stage416610 death/reload branch, with source fader counters.
 * Field/stage-clear/menu/Trial-specific/game-over screens are separate systems.*/
typedef struct {
    uint8_t state,request,fade_delay,fade_active,fade_alpha;
    uint16_t fade_value;
    int8_t lives;
    uint32_t restart_count;
} SA3DSStageRestart;
typedef struct {
    uint8_t run_world,process_requests,controls_enabled,clear_world,clear_rings,reload_stage;
    uint8_t game_over_pending,fade_draw;
} SA3DSStageRestartResult;
void sa3ds_stage_restart_init(SA3DSStageRestart *flow); /*425AF0 fresh lives4*/
void sa3ds_stage_restart_add_lives(SA3DSStageRestart *flow,int16_t amount);
/*Call before source world tasks; display_interval models03B1118C, capped6.
 * Native60Hz ordinary gameplay supplies1. Fades preserve source low16 returns.*/
int sa3ds_stage_restart_begin(SA3DSStageRestart *flow,unsigned display_interval,
    SA3DSStageRestartResult *out);
/*Only source state15 checks pending requests AFTER the world task pass. Pass
 * the begin result, even if begin changed4->15 during the fade completion.*/
int sa3ds_stage_restart_end(SA3DSStageRestart *flow,const SA3DSStageRestartResult *begin);
#endif
