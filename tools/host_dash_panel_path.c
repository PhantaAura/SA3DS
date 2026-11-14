/* Isolated static-terrain trajectories after a direct Act 1 boost.
 * Other stage tasks, controller input, and retail execution are outside this replay. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "emerald_coast_residency.h"
#include "dash_panel.h"
#include "spring_player.h"
#include "steam_flat.h"
#include "movement.h"
static void *read_blob(const char *path,size_t *n){
 FILE *f=fopen(path,"rb");if(!f)exit(2);
 fseek(f,0,SEEK_END);*n=ftell(f);rewind(f);
 void *p=malloc(*n);if(!p||fread(p,1,*n,f)!=*n)exit(2);fclose(f);return p;
}
int main(int argc,char **argv){
 if(argc!=3){fprintf(stderr,"usage: %s owner_collision.sa3dspak owner_movement.sa3dspak\n",argv[0]);return 2;}
 size_t cn,mn;void *c=read_blob(argv[1],&cn),*m=read_blob(argv[2],&mn);
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
   SA3DSSpringTrig trig;for(int axis=0;axis<3;axis++){double a=p.angles[axis]*(6.283185307179586/65536.0);trig.sine[axis]=sin(a);trig.cosine[axis]=cos(a);}
   SA3DSDashPanelParameters params;
   if(!sa3ds_dash_panel_parameters(&p,&params)||!sa3ds_dash_panel_apply(&s,&p,&trig,&params,SA3DS_EC_CONTROLLER_SUPPORT))return 2;
   float lowest=s.pos.y;unsigned tick,air=0;
   for(tick=0;tick<240;tick++){
    if(sa3ds_ec_residency_needs_recenter(&col,s.pos))
     sa3ds_ec_residency_recenter(&col,c,cn,roundf(s.pos.x/256)*256,roundf(s.pos.z/256)*256,&s);
    SA1Input in={0};SA3DSSpringPendingCommand cmd={0};
    sa3ds_spring_player_begin(&s,&cmd,&in);
    int was_grounded=s.grounded;
    SA1Vec3 prev=s.pos;col.reference_y=s.pos.y;
    sa1_physics_step_steam_sonic_flat(&s,&in,0,&col.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
    if(s.sim_frame==0)break;
    sa3ds_spring_player_tail(&s);
    sa3ds_ec_collision_resolve_walls(&col,prev,&s);
    if(was_grounded!=s.grounded)
     printf("\n  tick=%u grounded=%d pos=%.2f,%.2f,%.2f vel=%.2f,%.2f,%.2f flags=%08x",
       tick,s.grounded,s.pos.x,s.pos.y,s.pos.z,s.vel.x,s.vel.y,s.vel.z,s.surface_flags);
    lowest=fminf(lowest,s.pos.y);air+=!s.grounded;
   }
   printf("\n  ticks=%u air=%u minY=%.2f grounded=%d speed=%.1f lock=%d end=%.1f,%.1f,%.1f",tick,air,lowest,s.grounded,params.speed,params.control_timer,s.pos.x,s.pos.y,s.pos.z);
  }
  puts("");sa3ds_ec_collision_destroy(&col);
 }
 free(c);free(m);return 0;
}
