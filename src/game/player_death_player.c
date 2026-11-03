#include "player_death_player.h"
int sa3ds_player_death_player_command(SA1PlayerState*p,int alternate,uint16_t*sound,uint8_t*channel){
    if(!p)return 0;
    SA3DSPlayerDamageState s={.status=(uint16_t)(p->pc2004_status_flags|(p->grounded?1:0)|(p->ball_form?0x100:0)),.next_command=50};
    if(!sa3ds_player_death_command(&s,alternate,sound,channel))return 0;
    p->pc2004_action_state=s.action;p->pc2004_action_state_valid=true;p->ball_form=(s.status&0x100)!=0;
    p->pc2004_status_flags=(uint16_t)(s.status&~0x103u);p->action_state=SA1_ACTION_DEAD;
    p->animation_script_serial++;p->animation_script_request=s.animation;p->animation_script_valid=true;
    return 1;
}
