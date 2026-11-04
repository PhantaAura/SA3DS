#include "player_position_history.h"
#include <string.h>
void sa3ds_player_position_history_present(SA3DSPlayerPositionHistory*h,int present){if(h)h->present=present!=0;}
int sa3ds_player_position_history_record(SA3DSPlayerPositionHistory*h,const float p[3]){
 if(!h||!h->present||!p)return 0;
 h->cursor++;memcpy(h->position[h->cursor],p,sizeof(h->position[0]));return 1;
}
int sa3ds_player_position_history_read(const SA3DSPlayerPositionHistory*h,uint8_t offset,float out[3]){
 if(!h||!h->present)return 0;
 if(out)memcpy(out,h->position[(uint8_t)(h->cursor-offset)],sizeof(h->position[0]));
 return 1;
}
int sa3ds_player_position_history_write(SA3DSPlayerPositionHistory*h,uint8_t offset,const float p[3]){
 if(!h||!h->present)return 0;
 if(p)memcpy(h->position[(uint8_t)(h->cursor-offset)],p,sizeof(h->position[0]));
 return 1;
}
