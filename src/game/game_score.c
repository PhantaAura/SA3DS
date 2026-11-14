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
    *clear=(SA3DSClearScore){.remaining={score,time_bonus,rings*10u}};
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
