#include "whale_actor.h"
#include <math.h>
#include <string.h>
int sa3ds_whale_actor_init(SA3DSWhaleActor*a,const float p[3],int32_t yaw,
 const float previous[3],const SA3DSWhaleMotionServices*s){
 if(!a||!previous)return 0;
 SA3DSWhaleActor next={0};
 for(unsigned k=0;k<3;k++){if(!isfinite(previous[k]))return 0;next.shared_tip[k]=previous[k];}
 if(!sa3ds_whale_motion_init(&next.motion,p,yaw,s))return 0;
 next.collision_disabled=1;*a=next;return 1;
}
int sa3ds_whale_actor_tick(SA3DSWhaleActor*a,const SA3DSWhaleTipAssets*tip,
 const SA3DSWhaleMotionInput*input,uint8_t collision_flag8,
 const SA3DSWhaleActorServices*s,SA3DSWhaleActorResult*out){
 if(!a||!tip||!input||!s||!s->phase_requests||!out)return 0;
 memset(out,0,sizeof(*out));
 if(!sa3ds_whale_motion_phase(&a->motion,input,&s->motion,&out->phase))return 0;
 if(out->phase.clear_collision_disable)a->collision_disabled=0;
 const SA3DSWhaleMotionResult*r=&out->phase;
 if(r->destroy||r->clear_collision_disable||r->allow_free_camera||r->surface_splash||r->cycle_splash||r->rumble||r->sound2f)
  s->phase_requests(s->phase_context,r,a->shared_tip);
 if(r->stop_common_tail)return 1;
 if(!sa3ds_whale_tip(&a->motion,tip,&s->motion,a->shared_tip,NULL))return 0;
 SA3DSWhaleCommonInput common={.kill_request=input->kill_request,.collision_flag8=collision_flag8};
 memcpy(common.tip,a->shared_tip,sizeof(common.tip));
 return sa3ds_whale_common(&a->motion,&common,&s->common,&out->common);
}
