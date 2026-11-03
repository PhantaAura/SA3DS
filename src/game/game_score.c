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
