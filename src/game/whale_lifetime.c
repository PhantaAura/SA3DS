#include "whale_lifetime.h"
#include <math.h>
#include <string.h>
int sa3ds_whale_spawn(SA3DSWhaleShared*w,const float p[3],int32_t yaw,const SA3DSWhaleLifetimeServices*s){
 if(!w||!p||!s||!s->defer_free||!s->allocate||!s->set_spawn_data)return 0;
 for(unsigned k=0;k<3;k++)if(!isfinite(p[k]))return 0;
 if(w->current_task)s->defer_free(s->context,w->current_task);
 w->kill_request=0;uint32_t task=s->allocate(s->context);w->current_task=task;
 if(!task)return 0; /*Source would dereference null; port reports failure.*/
 s->set_spawn_data(s->context,task,p,yaw);return 1;
}
int sa3ds_whale_kill_request(SA3DSWhaleShared*w){
 if(!w||!w->current_task)return 0;
 w->kill_request=1;return 1;
}
int sa3ds_whale_shared_destruct(SA3DSWhaleShared*w,uint32_t task){
 if(!w||!task||w->current_task!=task)return 0;
 w->current_task=0;memset(w->tip,0,sizeof(w->tip));return 1;
}
