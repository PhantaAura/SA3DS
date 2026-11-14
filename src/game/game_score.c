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

void sa3ds_clear_score_begin(SA3DSClearScore *clear,uint32_t score,unsigned rings,
    unsigned minutes,unsigned seconds){
    uint32_t time_bonus=5000;
    if(minutes){
        unsigned elapsed=minutes<9&&seconds<60?minutes*60+seconds:500;
        time_bonus=elapsed<500?5000-elapsed*10:0;
    }
    *clear=(SA3DSClearScore){.remaining={score,time_bonus,rings*10u},
        .delay=309}; /* Init, two opening states, and six 21-tick reveals. */
}

int sa3ds_clear_score_tick(SA3DSClearScore *clear,int skip){
    if(skip&&clear->delay<=126){
        for(unsigned i=0;i<3;i++){
            clear->total+=clear->remaining[i];
            clear->remaining[i]=0;
        }
        clear->delay=0;
        return 0;
    }
    if(clear->delay){clear->delay--;return 0;}
    static const unsigned steps[]={46,83,7};
    for(unsigned i=0;i<3;i++){
        if(!clear->remaining[i])continue;
        uint32_t amount=clear->remaining[i]<steps[i]?clear->remaining[i]:steps[i];
        clear->remaining[i]-=amount;
        clear->total+=amount;
        return 1;
    }
    return 0;
}
