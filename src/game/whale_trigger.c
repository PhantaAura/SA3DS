#include "whale_trigger.h"
#include <math.h>
#include <string.h>
static int begin(SA3DSWhaleTrigger*s,const SA3DSWhaleTriggerInput*i,SA3DSWhaleTriggerResult*out){
 if(!s||!i||!out||i->inside_player>2)return 0;
 memset(out,0,sizeof(*out));return 1;
}
int sa3ds_whale_summon_tick(SA3DSWhaleTrigger*s,const SA3DSWhaleTriggerInput*i,SA3DSWhaleTriggerResult*out){
 if(!begin(s,i,out))return 0;
 if(i->clipped)return 1;
 if(i->debug_mode){out->debug_display=1;return 1;}
 if(s->action==0){if(i->inside_player==1)s->action=1;}
 else if(s->action==1){out->summon=1;s->action=2;}
 return 1;
}
int sa3ds_whale_kill_tick(SA3DSWhaleTrigger*s,const SA3DSWhaleTriggerInput*i,SA3DSWhaleTriggerResult*out){
 if(!begin(s,i,out))return 0;
 if(i->clipped)return 1;
 if(i->debug_mode){out->debug_display=1;return 1;}
 if(s->action==0&&i->inside_player)out->request_kill=1;
 return 1;
}
int sa3ds_whale_summon_position(const float p[3],const float offset[3],float out[3]){
 if(!p||!offset||!out)return 0;
 float result[3];for(unsigned k=0;k<3;k++){result[k]=p[k]+offset[k];if(!isfinite(result[k]))return 0;}
 memcpy(out,result,sizeof(result));return 1;
}
