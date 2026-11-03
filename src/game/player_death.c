#include "player_death.h"
int sa3ds_player_death_task_tick(SA3DSPlayerDeathTask*t,int special,int parent,SA3DSPlayerDeathTaskResult*r){
    if(!t||!r)return 0;
    *r=(SA3DSPlayerDeathTaskResult){0};if(!t->active)return 1;
    uint32_t previous=t->counter;t->counter=previous+1u;
    if(previous>120u){r->stage_request=2;r->parent_status4=parent!=0;r->audio_cleanup=r->destroy=1;t->active=0;}
    else if(special){r->destroy=1;t->active=0;}
    return 1;
}
int sa3ds_player_death_command(SA3DSPlayerDamageState*s,int alternate,uint16_t*sound,uint8_t*channel){
    if(!s||!sound||!channel||!(s->status&0x1000)||s->next_command!=50)return 0;
    s->action=86;s->animation=86;s->status&=0xdaffu;
    *sound=alternate?0x7fe:0x5de;*channel=alternate?1:0;return 1;
}

int sa3ds_player_protection_tick(SA3DSPlayerProtectionTask*t,uint16_t*flags,uint8_t*destroy){
    if(!t||!destroy)return 0;
    *destroy=0;if(!t->active)return 1;
    t->counter=(uint16_t)(t->counter+1u);
    if(t->counter>120u){
        if(flags)*flags&=(uint16_t)~0x8000u;
        t->active=0;*destroy=1;
    }else if(flags)*flags|=0x8000u;
    return 1;
}
