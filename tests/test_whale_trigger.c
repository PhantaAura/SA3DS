#include "whale_trigger.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
 SA3DSWhaleTrigger s={0};SA3DSWhaleTriggerInput i={0};SA3DSWhaleTriggerResult r;
 assert(sa3ds_whale_summon_tick(&s,&i,&r)&&!s.action&&!r.summon);
 i.inside_player=2;assert(sa3ds_whale_summon_tick(&s,&i,&r)&&!s.action); /*summon requires player0*/
 i.inside_player=1;i.clipped=1;assert(sa3ds_whale_summon_tick(&s,&i,&r)&&!s.action&&!r.summon);
 i.clipped=0;i.debug_mode=1;assert(sa3ds_whale_summon_tick(&s,&i,&r)&&!s.action&&r.debug_display);
 i.debug_mode=0;assert(sa3ds_whale_summon_tick(&s,&i,&r)&&s.action==1&&!r.summon); /*separate source update*/
 i.inside_player=0;assert(sa3ds_whale_summon_tick(&s,&i,&r)&&s.action==2&&r.summon); /*4F88A0 always0; no second sphere test or extra selection gate*/
 i.inside_player=1;assert(sa3ds_whale_summon_tick(&s,&i,&r)&&s.action==2&&!r.summon);
 s.action=0;i.inside_player=2;
 for(unsigned n=0;n<3;n++)assert(sa3ds_whale_kill_tick(&s,&i,&r)&&r.request_kill&&s.action==0); /*both players/repeated request*/
 i.clipped=1;assert(sa3ds_whale_kill_tick(&s,&i,&r)&&!r.request_kill);
 i.clipped=0;i.debug_mode=1;assert(sa3ds_whale_kill_tick(&s,&i,&r)&&r.debug_display&&!r.request_kill);
 i.debug_mode=0;i.inside_player=0;assert(sa3ds_whale_kill_tick(&s,&i,&r)&&!r.request_kill);
 s.action=3;i.inside_player=1;assert(sa3ds_whale_kill_tick(&s,&i,&r)&&!r.request_kill);
 float p[3]={11,19,23},offset[3]={-2,6,-90},out[3];
 assert(sa3ds_whale_summon_position(p,offset,out)&&out[0]==9&&out[1]==25&&out[2]==-67);
 assert(sa3ds_whale_summon_position(p,offset,p)&&p[0]==9&&p[1]==25&&p[2]==-67);
 offset[0]=INFINITY;assert(!sa3ds_whale_summon_position(p,offset,out));
 puts("Whale triggers: PASS source summon player0/delayed one-shot,kill both players/repeated request,clip/debug gates and raw offset; native chase absent");
}
