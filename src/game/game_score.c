#include "game_score.h"
#include <string.h>
static int32_t signed32(uint32_t u){int32_t n;memcpy(&n,&u,4);return n;}
void sa3ds_game_score_add(SA3DSGameScore*s,int32_t award){
    if(!s)return;
    if(s->chain_timer<1)s->chain_amount=0;
    else{int32_t n=signed32((uint32_t)s->chain_amount+(uint32_t)award);s->chain_amount=n>500?500:n;}
    s->chain_timer=120;s->total+=(uint32_t)award;
}
void sa3ds_game_score_tick(SA3DSGameScore*s,int enabled){
    if(!s||!enabled)return;
    int old_negative=s->chain_timer<0;
    s->chain_timer=signed32((uint32_t)s->chain_timer-1u);
    if(old_negative)s->chain_timer=0;
}

int sa3ds_clear_record_update(SA3DSClearRecord *record,uint32_t score,
    const uint8_t time[3],uint16_t rings){
    uint32_t ticks=((uint32_t)time[0]*60+time[1])*60+time[2];
    int best_score=signed32(score)>signed32(record->score);
    if(best_score)record->score=score;
    if(ticks<record->time_ticks)record->time_ticks=ticks;
    if(rings>record->rings)record->rings=rings;
    return best_score;
}

int sa3ds_clear_emblem(SA3DSClearRecord *record,const uint8_t time[3],
    uint16_t rings,int *awarded){
    *awarded=0;
    for(int mission=2;mission>=0;mission--){
        unsigned bit=1u<<mission;
        if(record->emblems&bit)continue;
        unsigned ticks=((unsigned)time[0]*60+time[1])*60+time[2];
        *awarded=mission==2||(mission==1?rings>=50:ticks<7200);
        if(*awarded)record->emblems|=(uint8_t)bit;
        return mission;
    }
    return -1;
}

unsigned sa3ds_clear_exit_tick(SA3DSClearExit *exit,int mission,int awarded,int award_busy){
    switch(exit->phase){
    case 0:
        if(exit->ticks++==20){
            *exit=(SA3DSClearExit){.phase=1};
            return SA3DS_CLEAR_AWARD;
        }
        break;
    case 1:
        if(++exit->ticks>(mission<0?120u:150u))
            *exit=(SA3DSClearExit){.phase=awarded?2u:4u};
        break;
    case 2:
        if(++exit->ticks>30&&!award_busy)*exit=(SA3DSClearExit){.phase=3};
        break;
    case 3:
        if(++exit->ticks>90)*exit=(SA3DSClearExit){.phase=4};
        break;
    case 4:
        *exit=(SA3DSClearExit){.phase=5};
        break;
    case 5:
        if(++exit->ticks==4)return SA3DS_CLEAR_LEAVE;
        break;
    }
    return 0;
}

void sa3ds_clear_score_begin(SA3DSClearScore *clear,uint32_t score,unsigned rings,
    unsigned minutes,unsigned seconds){
    uint32_t time_bonus=5000;
    if(minutes){
        unsigned elapsed=minutes<9&&seconds<60?minutes*60+seconds:500;
        time_bonus=elapsed<500?5000-elapsed*10:0;
    }
    *clear=(SA3DSClearScore){.remaining={score,time_bonus,rings*10u},
        .final_score=score+time_bonus+rings*10u};
}

unsigned sa3ds_clear_score_tick(SA3DSClearScore *clear,int advance){
    unsigned phase=clear->phase;
    if(advance&&clear->timer>10&&phase>=3&&(phase<10||(phase==10&&clear->timer<170))){
        if(phase<10){
            for(unsigned i=0;i<3;i++){
                clear->total+=clear->remaining[i];
                clear->remaining[i]=0;
            }
            clear->phase=10;
            clear->timer=179; /* The same update executes case 10. */
            clear->visible=0;
        }else{
            clear->phase=12;
            clear->timer=10;
            clear->visible=0;
        }
        return 0;
    }
    switch(phase){
    case 0:
        clear->phase=1;
        clear->timer=180;
        break;
    case 1:
        if(clear->timer)clear->timer--;
        else clear->phase=2;
        break;
    case 2:
        clear->phase=3;
        clear->timer=60;
        break;
    case 3: case 4: case 5: case 6: case 7: case 8:
        clear->visible=(1u<<(phase-3))-1;
        if(clear->timer)clear->timer--;
        else{
            clear->phase++;
            clear->timer=phase==8?60:20;
            if(phase!=8)return SA3DS_CLEAR_REVEAL_SOUND;
        }
        break;
    case 9: {
        clear->visible=SA3DS_CLEAR_TOTAL|31u;
        static const unsigned steps[]={46,83,7};
        for(unsigned i=0;i<3;i++){
            if(!clear->remaining[i])continue;
            uint32_t amount=clear->remaining[i]<steps[i]?clear->remaining[i]:steps[i];
            clear->remaining[i]-=amount;
            clear->total+=amount;
            return SA3DS_CLEAR_TALLY_SOUND;
        }
        if(clear->timer)clear->timer--;
        else{
            clear->phase=10;
            clear->timer=180;
        }
        break;
    }
    case 10:
        clear->visible=0;
        if(clear->timer)clear->timer--;
        else{
            clear->phase=12;
            clear->timer=60;
        }
        break;
    }
    return 0;
}
