/* Reads owner packages externally. Normal source start and actual port physics;
 * input-only opening object test, not an independent retail or full-route run. */
#include "ring_replay.h"
#include "emerald_coast_residency.h"
#include "player_view.h"
#include "movement.h"
#include "steam_flat.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void *read_file(const char*path,size_t*n){
 FILE*f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long len=ftell(f);rewind(f);assert(len>0);
 void*p=malloc((size_t)len);assert(p&&fread(p,1,(size_t)len,f)==(size_t)len);fclose(f);*n=(size_t)len;return p;
}
int main(int argc,char**argv){
 if(argc!=4){fprintf(stderr,"usage: %s COLLISION ROUTE RING_PLACEMENTS\n",argv[0]);return 2;}
 size_t cn,rn,pn;void*cb=read_file(argv[1],&cn),*rb=read_file(argv[2],&rn),*pb=read_file(argv[3],&pn);
 SA3DSECRoute route;SA3DSRings rings;
 assert(sa3ds_ec_route_open(&route,rb,rn)&&sa3ds_rings_open(&rings,pb,pn));
 const SA3DSECStart*start=sa3ds_ec_route_start(&route,0);assert(start);
 SA3DSECCollision collision;assert(sa3ds_ec_collision_open_region(&collision,cb,cn,0,start->task_origin.x,start->task_origin.z));
 SA1PlayerState player;sa3ds_ec_residency_start(&collision,start,&player);
 unsigned tick=0;
 for(;tick<1200&&rings.collected_events<3;tick++){
  SA1Input input=sa3ds_view_input(sa3ds_ring_replay_input(&rings,&player,0,tick));
  if(sa3ds_ec_residency_needs_recenter(&collision,player.pos))assert(sa3ds_ec_residency_recenter(&collision,cb,cn,player.pos.x,player.pos.z,&player));
  SA1Vec3 previous=player.pos;uint64_t frame=player.sim_frame;
  collision.reference_y=player.pos.y;SA1SteamFlatDebug debug={0};
  sa1_physics_step_steam_sonic_flat(&player,&input,0,&collision.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,&debug);
  assert(!(frame&&player.sim_frame==1)); /* no death/teleport may pass this route test */
  sa3ds_ec_collision_resolve_walls(&collision,previous,&player);
  SA1PlayerState before=player;SA3CMatrix actor;sa3ds_ec_model_transform(&player,&actor);
  unsigned collected=sa3ds_rings_tick(&rings,sa3ds_ec_task_origin(&player),
    (SA1Vec3){actor.m[12],actor.m[13],actor.m[14]},SA1_STEAM_SONIC_PARAMETER_EVIDENCE.rad,0,1,rings.clip_level);
  assert(!memcmp(&before,&player,sizeof(player)));
  if(collected||tick%120==0)printf("tick=%u pos=%.3f,%.3f,%.3f collected=%u rings=%d\n",tick,player.pos.x,player.pos.y,player.pos.z,collected,rings.rings);
 }
 int pass=rings.collected_events>=3&&rings.rings>=3;
 printf("Owner opening ring input replay: %s ticks=%u events=%u (port interaction; NOT RETAIL TRACE)\n",pass?"PASS":"FAIL",tick,rings.collected_events);
 sa3ds_ec_collision_destroy(&collision);sa3ds_rings_destroy(&rings);free(cb);free(rb);free(pb);return pass?0:1;
}
