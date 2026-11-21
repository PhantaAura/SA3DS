#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sonic_animation_runtime.h"
static int supported(int state){return state==2||state==3||state==9||state==10;}
int main(void){
 SA3DSSonicAnimationRuntime runtime={0};unsigned frame,request,index;int state;
 float phase,forward,scalar;unsigned checked=0,seeds=0,skipped=0;int ready=0;
 while(scanf("%u %u %u %d %f %f %f",&frame,&request,&index,&state,&phase,&forward,&scalar)==7){
  SA3DSSonicAnimationBinding binding;
  if(!supported(state)||!sa3ds_sonic_animation_binding((uint16_t)request,&binding)){ready=0;skipped++;continue;}
  if(!ready){
   if(state==2||index!=request){skipped++;continue;}
   memset(&runtime,0,sizeof runtime);runtime.initialized=1;runtime.last_sim_frame=frame;
   runtime.active_binding=binding;runtime.accumulated_sample_frame=phase;
   runtime.requested_runtime_id=(uint16_t)request;runtime.previous_action_state=SA1_ACTION_AIRBORNE;
   runtime.speed_scalar=scalar;runtime.speed_scalar_valid=1;ready=1;seeds++;continue;
  }
  SA1PlayerState player={0};player.sim_frame=frame;player.action_state=SA1_ACTION_AIRBORNE;
  player.animation_forward_speed=forward;player.animation_forward_speed_valid=1;player.spindash_charge=scalar;
  /* Isolate PlayAnimation: original request/scalar are inputs. This does
     not compare the controller, terrain solver or animation selector. */
  player.pc2004_action_state=16;player.pc2004_action_state_valid=1;
  player.animation_script_valid=1;player.animation_script_request=(uint16_t)request;player.animation_script_serial=frame+1;
  SA3DSSonicAnimationObservation observation={0};observation.intent=SA3DS_SONIC_ANIM_SPINDASH_CHARGE;
  SA3DSSonicAnimationSelection result=sa3ds_sonic_animation_select(&runtime,&player,&observation);
  unsigned effective=state==2?index:request;
  float observed=state==2?result.transition_weight:result.sample_frame;
  if(result.requested_runtime_id!=request||result.binding.runtime_id!=effective||result.transition_active!=(state==2)||fabsf(observed-phase)>0.00001f){
   printf("FIRST_DIVERGENCE frame=%u request=%u source_index=%u source_state=%d source_phase=%.9g port_id=%u port_blending=%d port_phase=%.9g checked_before=%u\n",frame,request,index,state,phase,result.binding.runtime_id,result.transition_active,observed,checked);
   return 1;
  }
  checked++;
 }
 printf("PLAYBACK_COMPONENT checked=%u seeded_spans=%u excluded=%u; selector/physics/pose parity not established\n",checked,seeds,skipped);
 return checked?0:2;
}
