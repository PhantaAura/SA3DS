#include "stage_restart.h"
#include <string.h>
static int signed_byte(uint8_t u){int8_t n;memcpy(&n,&u,1);return n;}
void sa3ds_stage_restart_init(SA3DSStageRestart*f){if(f)*f=(SA3DSStageRestart){.state=15,.lives=4};}
void sa3ds_stage_restart_add_lives(SA3DSStageRestart*f,int16_t amount){
    if(!f)return;
    int next=signed_byte((uint8_t)((uint8_t)f->lives+(uint8_t)amount));
    if(amount>0&&next<0)next=127;
    f->lives=(int8_t)next;
}
static void fade_draw(SA3DSStageRestart*f){f->fade_alpha=f->fade_value>254?255:(uint8_t)f->fade_value;}
/*42C650's low16 result:0 while active, -1 only on the call AFTER completion.*/
static int fade_out(SA3DSStageRestart*f,unsigned interval){
    if(f->fade_delay){f->fade_delay--;return 0;}
    fade_draw(f);
    if(!f->fade_active){f->fade_alpha=255;return -1;}
    if(f->fade_value==255)f->fade_active=0;
    else{
        if(f->fade_value>248){f->fade_value=255;return 0;}
        f->fade_value=(uint16_t)(f->fade_value+6);
        for(unsigned n=1;n<interval;n++)if(f->fade_value<255)f->fade_value=(uint16_t)(f->fade_value+6);
    }
    return 0;
}
/*42C570 returns -1 during its initial delay AND when already inactive. Stage3
 * reloads at the first0; stage4 becomes15 at the later -1.*/
static int fade_in(SA3DSStageRestart*f,unsigned interval){
    if(f->fade_delay){fade_draw(f);f->fade_delay--;return -1;}
    if(!f->fade_active)return -1;
    if(f->fade_value<7){f->fade_alpha=0;f->fade_active=0;}
    else{
        fade_draw(f);f->fade_value=(uint16_t)(f->fade_value-6);
        for(unsigned n=1;n<interval;n++)if(f->fade_value>5)f->fade_value=(uint16_t)(f->fade_value-6);
    }
    return 0;
}
int sa3ds_stage_restart_begin(SA3DSStageRestart*f,unsigned interval,SA3DSStageRestartResult*r){
    if(!f||!r)return 0;
    *r=(SA3DSStageRestartResult){0};if(interval>6)interval=6;
    switch(f->state){
    case 15:r->process_requests=1;r->run_world=f->request==0;r->controls_enabled=r->run_world;return 1;
    case 6:
        r->fade_draw=1;
        if(fade_out(f,interval)!=0){r->clear_world=r->clear_rings=1;sa3ds_stage_restart_add_lives(f,-1);f->request=0;f->state=2;}
        else r->run_world=1;
        return 1;
    case 2:f->fade_active=1;f->fade_delay=5;f->fade_value=291;fade_draw(f);f->state=3;
        /*Source case2 falls through into3 in the same game-manager call.*/
        /*fallthrough*/
    case 3:
        r->fade_draw=1;
        if(fade_in(f,interval)==0){r->reload_stage=1;f->state=4;f->restart_count++;}
        return 1;
    case 4:
        r->fade_draw=r->run_world=1;
        if(fade_in(f,interval)!=0)f->state=15;
        return 1;
    case 8:r->game_over_pending=1;return 1; /*real game-over executor pending*/
    default:return 0;
    }
}
int sa3ds_stage_restart_end(SA3DSStageRestart*f,const SA3DSStageRestartResult*begin){
    if(!f||!begin)return 0;
    if(!begin->process_requests||f->request!=2)return 1;
    f->request=0;
    if(f->lives<1){f->state=8;return 1;}
    f->state=6;f->fade_active=1;f->fade_delay=1;f->fade_value=f->fade_alpha;fade_draw(f);return 1;
}
