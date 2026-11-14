/* Isolated static-terrain trajectories after a direct Act 1 boost.
 * Optional chaining runs nearby unfiltered dash-panel tasks after movement.
 * With a SET file, the WALL sweep runs as an isolated response experiment.
 * Other stage tasks, controller input, and retail execution are outside this replay. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "emerald_coast_residency.h"
#include "dash_panel.h"
#include "ccl_wall.h"
#include "spring_player.h"
#include "steam_flat.h"
#include "movement.h"
#include <string.h>
static void *read_blob(const char *path,size_t *n){
 FILE *f=fopen(path,"rb");if(!f)exit(2);
 fseek(f,0,SEEK_END);*n=ftell(f);rewind(f);
 void *p=malloc(*n);if(!p||fread(p,1,*n,f)!=*n)exit(2);fclose(f);return p;
}
static SA3DSSpringTrig panel_trig(const SA3DSMovementPlacement *placement){
 SA3DSSpringTrig trig;
 for(unsigned axis=0;axis<3;axis++){
  double angle=placement->angles[axis]*(6.283185307179586/65536.0);
  trig.sine[axis]=sin(angle);
  trig.cosine[axis]=cos(angle);
 }
 return trig;
}
/* Host-only decoded form 9 placements. Inputs are little-endian SET rows. */
static unsigned wall_sweep(const unsigned char *rows, unsigned count,
    SA1Vec3 previous, SA1PlayerState *player){
 unsigned hits=0;
 for(unsigned i=0;i<count;i++){
  const unsigned char *row=rows+32*i;
  if(row[0]!=20||row[1]!=0)continue;
  int16_t yaw;
  float values[6];
  memcpy(&yaw,row+4,sizeof yaw);
  memcpy(values,row+8,sizeof values);
  SA1Vec3 center={values[0],values[1],values[2]};
  SA1Vec3 extents={values[3]+10,values[4]+10,values[5]+10};
  double angle=yaw*(6.283185307179586/65536.0);
  float sn=(float)sin(angle),cs=(float)cos(angle);
  SA1Vec3 current=sa3ds_ec_task_origin(player);
  SA1Vec3 a=sa1_vec3_sub(previous,center),b=sa1_vec3_sub(current,center);
  SA1Vec3 local_start={a.x*cs-a.z*sn,a.y,a.x*sn+a.z*cs};
  SA1Vec3 local_end={b.x*cs-b.z*sn,b.y,b.x*sn+b.z*cs};
  SA1Vec3 corrected;
  if(sa3ds_ccl_wall_sweep(local_start,local_end,4,4,extents,&corrected)==SA3DS_WALL_MISS)continue;
  /* 41A580 writes sample X/Z through the yaw transform. This experiment
     omits native CCL list ordering and the source task-position service. */
  player->pos.x+=center.x+corrected.x*cs+corrected.z*sn-current.x;
  player->pos.z+=center.z+corrected.z*cs-corrected.x*sn-current.z;
  hits++;
 }
 return hits;
}
int main(int argc,char **argv){
 int chain=(argc==4||argc==5)&&!strcmp(argv[3],"--chain");
 if(argc!=3&&!chain){fprintf(stderr,"usage: %s owner_collision.sa3dspak owner_movement.sa3dspak [--chain [SET0100S.BIN]]\n",argv[0]);return 2;}
 size_t cn,mn;void *c=read_blob(argv[1],&cn),*m=read_blob(argv[2],&mn);
 size_t wall_size=0;
 unsigned char *wall_set=argc==5?read_blob(argv[4],&wall_size):NULL;
 uint32_t wall_count=0;
 if(wall_set){
  if(wall_size<32)return 2;
  memcpy(&wall_count,wall_set,sizeof wall_count);
  if(wall_count>1023||wall_size!=32+(size_t)wall_count*32)return 2;
 }
 SA3DSMovementObjects objects;if(!sa3ds_movement_objects_open(&objects,m,mn))return 2;
 SA3DSECCollisionArchive arc;if(!sa3ds_ec_collision_archive_open(&arc,c,cn))return 2;
 for(unsigned i=0;i<objects.count;i++){
  SA3DSMovementPlacement p;if(!sa3ds_movement_objects_placement(&objects,i,&p))return 2;
  if(p.act||(p.type&4095)!=3)continue;
  SA3DSECCollision col;
  if(!sa3ds_ec_collision_open_archive_region(&col,&arc,0,p.position.x,p.position.z)){printf("row=%u no region\n",p.source_row);continue;}
  col.reference_y=p.position.y+8;
  SA1Surface floor=sa3ds_ec_collision_surface(&col,p.position.x,p.position.z);
  printf("row=%u pos=%.1f,%.1f,%.1f yaw=%d floor=%d ",p.source_row,p.position.x,p.position.y,p.position.z,p.angles[1],floor.valid);
  if(floor.valid){
   SA1PlayerState s={.pos={p.position.x,floor.ground_y+SA3DS_EC_CONTROLLER_SUPPORT,p.position.z},.grounded=1,.ground_normal=floor.normal,.surface_id=floor.surface_id};
   col.world.reset_min_y=-80;
   col.world.reset_min_x=col.world.reset_min_z=-10000;col.world.reset_max_x=col.world.reset_max_z=10000;
   SA3DSSpringTrig trig=panel_trig(&p);
   SA3DSDashPanelParameters params;
   if(!sa3ds_dash_panel_parameters(&p,&params)||!sa3ds_dash_panel_apply(&s,&p,&trig,&params,SA3DS_EC_CONTROLLER_SUPPORT))return 2;
   SA3DSDashPanel actors[2046]={0};
   unsigned char pending[2046]={0};
   actors[i]=(SA3DSDashPanel){.initialized=1,.cooldown=params.lockout,.last_player=0};
   float lowest=s.pos.y;unsigned tick,air=0;
   SA1Vec3 reset_from={0};
   for(tick=0;tick<240;tick++){
    if(sa3ds_ec_residency_needs_recenter(&col,s.pos))
     sa3ds_ec_residency_recenter(&col,c,cn,roundf(s.pos.x/256)*256,roundf(s.pos.z/256)*256,&s);
    SA1Input in={0};SA3DSSpringPendingCommand cmd={0};
    sa3ds_spring_player_begin(&s,&cmd,&in);
    int was_grounded=s.grounded;
    SA1Vec3 previous_task=sa3ds_ec_task_origin(&s);
    SA1Vec3 prev=s.pos;col.reference_y=s.pos.y;
    sa1_physics_step_steam_sonic_flat(&s,&in,0,&col.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
    if(s.sim_frame==0){reset_from=prev;break;}
    sa3ds_spring_player_tail(&s);
    sa3ds_ec_collision_resolve_walls(&col,prev,&s);
    if(chain){
     SA3CMatrix model;
     sa3ds_ec_model_transform(&s,&model);
     SA1Vec3 center={model.m[12],model.m[13],model.m[14]};
     for(uint32_t j=0;j<objects.count;j++){
      SA3DSMovementPlacement next;
      if(!sa3ds_movement_objects_placement(&objects,j,&next))return 2;
      /* Only unconditional SET rows are eligible in this isolated replay. */
      if(next.act||next.type!=3)continue;
      SA1Vec3 delta=sa1_vec3_sub(center,next.position);
      if(sa1_vec3_dot(delta,delta)>360100.f){
       actors[j]=(SA3DSDashPanel){0};
       pending[j]=0;
       continue;
      }
      SA3DSDashPanelInput input={.first_hit_player=pending[j]?0:-1,
       .player_ground_status_bit0=s.grounded};
      SA3DSDashPanelResult result;
      SA3DSSpringTrig orientation=panel_trig(&next);
      if(!sa3ds_dash_panel_tick(&actors[j],&next,&input,&result))return 2;
      pending[j]=result.enqueue_collision&&sa3ds_dash_panel_primary_overlap(&next,&orientation,center,4.f)==1;
      if(result.boost&&sa3ds_dash_panel_apply(&s,&next,&orientation,&result.parameters,SA3DS_EC_CONTROLLER_SUPPORT))
       printf("\n  tick=%u boost row=%u",tick,next.source_row);
     }
    }
    if(wall_set){
     unsigned hits=wall_sweep(wall_set+32,wall_count,previous_task,&s);
     if(hits)printf("\n  tick=%u wall contacts=%u",tick,hits);
    }
    if(was_grounded!=s.grounded)
     printf("\n  tick=%u grounded=%d pos=%.2f,%.2f,%.2f vel=%.2f,%.2f,%.2f flags=%08x",
       tick,s.grounded,s.pos.x,s.pos.y,s.pos.z,s.vel.x,s.vel.y,s.vel.z,s.surface_flags);
    lowest=fminf(lowest,s.pos.y);air+=!s.grounded;
   }
   printf("\n  outcome=%s ticks=%u air=%u minY=%.2f grounded=%d speed=%.1f lock=%d end=%.1f,%.1f,%.1f",tick==240?"window-complete":"physics-reset",tick,air,lowest,s.grounded,params.speed,params.control_timer,s.pos.x,s.pos.y,s.pos.z);
   if(tick<240)printf(" resetFrom=%.2f,%.2f,%.2f",reset_from.x,reset_from.y,reset_from.z);
  }
  puts("");sa3ds_ec_collision_destroy(&col);
 }
 free(wall_set);free(c);free(m);return 0;
}
