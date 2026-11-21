/* Isolated static-terrain trajectories after a direct Act 1 boost.
 * Optional chaining runs nearby unfiltered dash-panel tasks after movement.
 * With a SET file, the WALL sweep runs as an isolated response experiment.
 * Other stage tasks, controller input, and retail execution are outside this replay. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "emerald_coast_residency.h"
#include "dash_panel.h"
#include "pc2004_sphere_sweep.h"
#include "pc2004_collision_work.h"
#include "pc2004_motion.h"
#include "pc2004_task_frame.h"
#include "steam_player_parameters.h"
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
static void wall_sweep(const unsigned char *rows, unsigned count,
    SA1Vec3 previous, SA1PlayerState *player, unsigned tick){
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
  SA3DSWallResponse response=sa3ds_ccl_wall_sweep(local_start,local_end,4,4,extents,&corrected);
  if(response==SA3DS_WALL_MISS)continue;
  printf("\n  tick=%u wall row=%u response=%s",tick,i,response==SA3DS_WALL_HARD?"hard":"soft");
  /* 41A580 writes sample X/Z through the yaw transform. This experiment
     omits native CCL list ordering and the source task-position service. */
  player->pos.x+=center.x+corrected.x*cs+corrected.z*sn-current.x;
  player->pos.z+=center.z+corrected.z*cs-corrected.x*sn-current.z;
 }
}
/* Ordinary static Sonic confirmation probe. Only the candidate object
   is re-swept, using the same resident polygon iteration as the trace. */
static void report_fast_support_probe(const SA3DSECTriangle *triangles, size_t count,
    const float movement[3], SA1Vec3 start, SA1Vec3 task, SA1Vec3 up,
    SA1Vec3 forward, SA1Vec3 corrected, int32_t first_index, SA1Contact *confirmed){
 const SA1SteamPlayerParameters *parameters=&SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
 SA1Vec3 delta={movement[0],movement[1],movement[2]};
 if(sa1_vec3_dot(delta,forward)<parameters->dash_speed||
    (triangles[0].flags&0x4000))return;
 float vertical=sa1_vec3_dot(delta,up);
 SA1Vec3 origin_task=sa1_vec3_add(task,sa1_vec3_sub(corrected,start));
 SA1Vec3 center=sa1_pc2004_fast_ground_probe_center(origin_task,up,
     4.0f,vertical,parameters->pos_error);
 const float origin[]={center.x,center.y,center.z};
 SA3DSSweepObject object={0};
 size_t face=0;
 for(size_t i=0;i<count&&object.count<SA3DS_PC2004_SWEEP_MAX_HITS;i++){
  const SA3DSECTriangle *t=&triangles[i];
  SA3DSSweepTriangle triangle={.v={{t->a.x,t->a.y,t->a.z},{t->b.x,t->b.y,t->b.z},
      {t->c.x,t->c.y,t->c.z}},.flags=t->flags};
  SA3DSSweepHit hit;
  if(!sa3ds_pc2004_sweep_triangle(&triangle,origin,movement,5.1f,&hit))continue;
  if(sa3ds_pc2004_sweep_record(&object,&hit))face=i;
 }
 printf(" fastReprobeHits=%u",object.count);
 if(object.count){
  SA3DSSweepHit selected=object.selected;
  SA1Vec3 corrected_sum={object.corrected_sum[0],object.corrected_sum[1],object.corrected_sum[2]};
  SA1Vec3 normal={selected.signed_normal[0],selected.signed_normal[1],selected.signed_normal[2]};
  const SA3DSECTriangle *triangle=&triangles[face];
  int speed_gate=sa1_pc2004_collision_support_speed(
      delta,normal,parameters->run_speed,vertical,1,triangle->flags);
  printf(" fastReprobeFace=%u/%u speedGate=%d",
      triangle->source_instance,triangle->source_triangle,speed_gate);
  if(!speed_gate)printf(" localVertical=%.6f",vertical);
  SA1Vec3 projected=sa1_pc2004_collision_support_position(
      sa1_vec3_scale(corrected_sum,1.0f/object.count),
      (SA1Vec3){selected.delta_xz[0],selected.delta_xz[1],selected.delta_xz[2]},
      (SA1Vec3){selected.plane_point[0],selected.plane_point[1],selected.plane_point[2]},
      normal);
  printf(" reprobeTask=%.3f,%.3f,%.3f",projected.x,projected.y,projected.z);
  if(speed_gate&&sa1_pc2004_collision_ground_side(up,normal,(SA1Vec3){0,-1,0},0))
   *confirmed=(SA1Contact){.point=projected,.normal=normal,
       .signed_distance=SA3DS_EC_CONTROLLER_SUPPORT,.surface_id=first_index+(int32_t)face,
       .surface_flags=triangle->flags,.valid=true};
 }
}

/* 453820 keeps at most 16 hits in collision face order per object,
   then selects the first hit with the lowest final plane height. */
static float report_source_selection(SA3DSECCollision *collision, unsigned tick,
    uint32_t instance, uint32_t face, unsigned count, const SA3DSSweepHit *hit,
    SA1Vec3 corrected_sum, SA1Vec3 previous_up, const float movement[3],
    const SA1PC2004CollisionCandidate *candidates){
 printf("\n  source object tick=%u polygon=%u/%u recorded=%u endHeight=%.4f",
     tick,instance,face,count,hit->end_height);
 SA1Vec3 motion={movement[0],movement[1],movement[2]};
 float clipped=sa1_pc2004_collision_prepass_length(previous_up,
     (SA1Vec3){0,-1,0},motion,1,candidates,count);
 printf(" prepassLength=%.4f->%.4f",sa1_vec3_length(motion),clipped);
 SA1Vec3 normal={hit->signed_normal[0],hit->signed_normal[1],hit->signed_normal[2]};
 SA1Vec3 tangent={hit->delta_xz[0],hit->delta_xz[1],hit->delta_xz[2]};
 SA1Vec3 point={hit->plane_point[0],hit->plane_point[1],hit->plane_point[2]};
 SA1Vec3 task=sa1_pc2004_collision_support_position(
     sa1_vec3_scale(corrected_sum,1.0f/count),tangent,point,normal);
 SA1Vec3 support_center=sa1_vec3_add(task,sa1_vec3_scale(normal,SA3DS_EC_CONTROLLER_SUPPORT));
 SA1Contact support=sa3ds_ec_collision_contact(collision,support_center,
     sa1_vec3_scale(normal,-1),2.35f);
 printf(" projectedTask=%.3f,%.3f,%.3f normalY=%.4f projectedSupport=%d",
     task.x,task.y,task.z,normal.y,support.valid);
 if(support.valid&&support.surface_id>=0){
  const SA3DSECTriangle *triangle=&collision->triangles[support.surface_id];
  printf(" projectedFace=%u/%u",triangle->source_instance,triangle->source_triangle);
 }
 return clipped;
}

/* 449500 re-sweeps each response object after first-pass travel clamps.
   Keep this observation separate from applying a mutable object response. */
static void report_response_sweep(const SA3DSECCollision *collision,unsigned tick,
    uint32_t instance,const float start[3],const float movement[3],float radius,
    SA1Vec3 up){
 SA3DSSweepObject object={0};
 const SA3DSECTriangle *selected=NULL;
 for(uint32_t i=0;i<collision->triangle_count&&object.count<SA3DS_PC2004_SWEEP_MAX_HITS;i++){
  const SA3DSECTriangle *t=&collision->triangles[i];
  if(t->source_instance!=instance)continue;
  SA3DSSweepTriangle triangle={.v={{t->a.x,t->a.y,t->a.z},{t->b.x,t->b.y,t->b.z},
      {t->c.x,t->c.y,t->c.z}},.flags=t->flags};
  SA3DSSweepHit hit;
  if(sa3ds_pc2004_sweep_triangle(&triangle,start,movement,radius,&hit)&&
     sa3ds_pc2004_sweep_record(&object,&hit))selected=t;
 }
 printf("\n  response sweep tick=%u object=%u hits=%u",tick,instance,object.count);
 if(selected){
  SA1Vec3 normal={object.selected.signed_normal[0],object.selected.signed_normal[1],
      object.selected.signed_normal[2]};
  printf(" face=%u alignment=%.6f groundSide=%d flags=%08x",selected->source_triangle,
      sa1_vec3_dot(up,normal),
      sa1_pc2004_collision_ground_side(up,normal,(SA1Vec3){0,-1,0},selected->flags),
      selected->flags);
  SA1Vec3 motion={movement[0],movement[1],movement[2]};
  float speed=sa1_vec3_length(motion);
  float toward=speed>0.0f?sa1_vec3_dot(sa1_vec3_scale(motion,1.0f/speed),normal):0.0f;
  float vertical=sa1_vec3_dot(motion,up);
  int speed_gate=sa1_pc2004_collision_support_speed(motion,normal,
      SA1_STEAM_SONIC_PARAMETER_EVIDENCE.run_speed,vertical,1,selected->flags);
  SA1Vec3 corrected={object.corrected_sum[0],object.corrected_sum[1],object.corrected_sum[2]};
  SA1Vec3 projected=sa1_pc2004_collision_support_position(
      sa1_vec3_scale(corrected,1.0f/object.count),
      (SA1Vec3){object.selected.delta_xz[0],object.selected.delta_xz[1],object.selected.delta_xz[2]},
      (SA1Vec3){object.selected.plane_point[0],object.selected.plane_point[1],object.selected.plane_point[2]},
      normal);
  printf(" speed=%.6f motionDot=%.6f speedGate=%d normal=%.6f,%.6f,%.6f baseTask=%.6f,%.6f,%.6f",
      speed,toward,speed_gate,normal.x,normal.y,normal.z,projected.x,projected.y,projected.z);
 }
}

/* This uses the port's movement and static candidates as sweep inputs.
   Clip propagation uses resident object order. The original broad phase
   and the second-pass collision response are outside this trace. */
static SA1Contact report_source_sweep(SA3DSECCollision *collision, unsigned tick,
    SA1Vec3 previous_task, SA1Vec3 previous_up, SA1Vec3 delta, SA1Vec3 end,
    uint32_t previous_instance, SA1Vec3 forward){
 const SA1SteamPlayerParameters *parameters=&SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
 float radius=parameters->height*0.5f;
 int fast=sa1_vec3_dot(delta,forward)>=parameters->dash_speed;
 float vertical_speed=sa1_vec3_dot(delta,previous_up);
 SA1Vec3 center=fast ? sa1_pc2004_fast_ground_probe_center(previous_task,previous_up,
     radius,vertical_speed,parameters->pos_error)
     : sa1_vec3_add(previous_task,sa1_vec3_scale(previous_up,radius));
 printf("\n  source sweep origin tick=%u centerHeight=%.4f fast=%d",tick,
     sa1_vec3_dot(sa1_vec3_sub(center,previous_task),previous_up),fast);
 const float start[]={center.x,center.y,center.z};
 float movement[]={delta.x,delta.y,delta.z};
 enum { OBJECT_LIMIT=1024 };
 uint32_t object_ids[OBJECT_LIMIT];
 size_t object_count=0, promoted=SIZE_MAX, previous_index=SIZE_MAX;
 SA1Contact confirmed={0};
 unsigned hits=0;
 SA3DSSweepObject object={0};
 uint32_t object_start=0;
 uint32_t instance=UINT32_MAX, selected_face=0;
 SA1PC2004CollisionCandidate candidates[SA3DS_PC2004_SWEEP_MAX_HITS];
 for(uint32_t i=0;i<=collision->triangle_count;i++){
  const SA3DSECTriangle *t=i<collision->triangle_count ? &collision->triangles[i] : NULL;
  if(!t||t->source_instance!=instance){
   if(object.count){
    SA1Vec3 corrected_sum={object.corrected_sum[0],object.corrected_sum[1],object.corrected_sum[2]};
    if(object_count==OBJECT_LIMIT)break;
    float current=sa1_vec3_length((SA1Vec3){movement[0],movement[1],movement[2]});
    float clipped=report_source_selection(collision,tick,instance,selected_face,
        object.count,&object.selected,corrected_sum,previous_up,movement,candidates);
    report_fast_support_probe(&collision->triangles[object_start],i-object_start,
        movement,center,previous_task,previous_up,forward,
        sa1_vec3_scale(corrected_sum,1.0f/object.count),(int32_t)object_start,&confirmed);
    if(clipped<current){
     for(unsigned axis=0;axis<3;axis++)movement[axis]*=clipped/current;
     promoted=object_count;
    }
    if(instance==previous_instance)previous_index=object_count;
    object_ids[object_count++]=instance;
   }
   if(!t)break;
   instance=t->source_instance;
   object_start=i;
   object=(SA3DSSweepObject){0};
  }
  if(object.count==SA3DS_PC2004_SWEEP_MAX_HITS||(t->flags&0x400002))continue;
  /* The primitive expands the triangle by four percent around its centroid. */
  float margin_x=radius+0.04f*(t->max_x-t->min_x);
  float margin_z=radius+0.04f*(t->max_z-t->min_z);
  if(t->max_x<fminf(center.x,center.x+delta.x)-margin_x||
     t->min_x>fmaxf(center.x,center.x+delta.x)+margin_x||
     t->max_z<fminf(center.z,center.z+delta.z)-margin_z||
     t->min_z>fmaxf(center.z,center.z+delta.z)+margin_z)continue;
  SA3DSSweepTriangle triangle={.v={{t->a.x,t->a.y,t->a.z},{t->b.x,t->b.y,t->b.z},
      {t->c.x,t->c.y,t->c.z}},.flags=t->flags};
  SA3DSSweepHit hit;
  if(!sa3ds_pc2004_sweep_triangle(&triangle,start,movement,radius,&hit))continue;
  SA1Vec3 normal={hit.signed_normal[0],hit.signed_normal[1],hit.signed_normal[2]};
  float port_distance=sa1_vec3_dot(sa1_vec3_sub(end,t->a),t->normal);
  printf("\n  source sweep tick=%u polygon=%u/%u alignment=%.4f height=%.3f->%.3f pointDistance=%.3f groundSide=%d",
      tick,t->source_instance,t->source_triangle,sa1_vec3_dot(normal,previous_up),
      hit.start_height,hit.end_height,port_distance,
      sa1_pc2004_collision_ground_side(previous_up,normal,(SA1Vec3){0,-1,0},0));
  SA1Vec3 corrected={hit.corrected[0],hit.corrected[1],hit.corrected[2]};
  candidates[object.count]=(SA1PC2004CollisionCandidate){normal,
      sa1_vec3_length(sa1_vec3_sub(corrected,center))};
  if(sa3ds_pc2004_sweep_record(&object,&hit))selected_face=t->source_triangle;
  hits++;
 }
 if(object_count){
  size_t order[OBJECT_LIMIT];
  sa1_pc2004_collision_response_order(object_count,promoted,previous_index,0,order);
  printf("\n  static response order tick=%u hitObjects=",tick);
  for(size_t i=0;i<object_count;i++)printf("%s%u",i?",":"",object_ids[order[i]]);
  if(object_count>1)for(size_t i=0;i<object_count;i++)
   report_response_sweep(collision,tick,object_ids[order[i]],start,movement,radius,previous_up);
 }
 if(!hits)printf("\n  source sweep tick=%u no polygon hit",tick);
 /* Applying an isolated confirmation is only comparable while no other
    object or prepass clamp needs to take part in the response. */
 return object_count==1&&promoted==SIZE_MAX ? confirmed : (SA1Contact){0};
}

static void apply_confirmed_support(SA1PlayerState *player, const SA1PlayerState *before,
    const SA1Contact *support, int task_angles){
 SA1Vec3 motion=player->vel;
 SA1SurfaceFrame frame=sa1_surface_frame_make(support->normal,player->surface_forward);
 if(task_angles){
  motion=player->pc2004_world_motion;
  SA1SurfaceFrame previous=sa1_surface_frame_make(before->ground_normal,before->surface_forward);
  uint16_t target_x,target_z;
  float forward_speed=sa1_vec3_dot(motion,previous.forward);
  if(sa1_pc2004_task_ordinary_support_target(&previous,support->normal,&target_x,&target_z)&&
      sa1_pc2004_task_approach_support(&previous,support->normal,target_x,target_z,forward_speed,
      &SA1_STEAM_SONIC_PARAMETER_EVIDENCE,NULL))frame=previous;
 }
 if(!player->grounded){
  player->action_state=before->action_state;
  player->pc2004_action_state=before->pc2004_action_state;
  player->pc2004_action_state_valid=before->pc2004_action_state_valid;
  player->animation_airborne_request=before->animation_airborne_request;
  player->animation_airborne_request_valid=before->animation_airborne_request_valid;
  player->jump_ticks=before->jump_ticks;
 }
 player->grounded=true;
 player->pos=sa1_vec3_add(support->point,sa1_vec3_scale(frame.up,SA3DS_EC_CONTROLLER_SUPPORT));
 SA1PC2004LocalVector local={
     sa1_vec3_dot(motion,frame.forward),sa1_vec3_dot(motion,frame.up),
     sa1_vec3_dot(motion,frame.right),
 };
 local=sa1_pc2004_grounded_reset_speed(local,0,SA1_STEAM_SONIC_PARAMETER_EVIDENCE.run_accel);
 player->vel=sa1_vec3_add(sa1_vec3_scale(frame.forward,local.forward),
     sa1_vec3_scale(frame.right,local.side));
 player->ground_normal=frame.up;
 player->ground_slope_radians=sa1_surface_slope_radians(frame.up);
 player->surface_forward=frame.forward;
 player->surface_right=frame.right;
 player->surface_frame_active=true;
 player->surface_id=support->surface_id;
 player->surface_flags=support->surface_flags;
 player->support_distance=support->signed_distance;
 player->support_up_y=support->normal.y;
 player->support_up_valid=true;
 player->yaw=atan2f(frame.forward.x,frame.forward.z);
}

int main(int argc,char **argv){
 int chain=(argc==4||argc==5)&&!strcmp(argv[3],"--chain");
 int task_angles=chain&&argc==5&&!strcmp(argv[4],"--apply-static-task-angles");
 int apply_support=task_angles||(chain&&argc==5&&!strcmp(argv[4],"--apply-static-support"));
 if(argc!=3&&!chain){fprintf(stderr,"usage: %s owner_collision.sa3dspak owner_movement.sa3dspak [--chain [SET0100S.BIN | --apply-static-support | --apply-static-task-angles]]\n",argv[0]);return 2;}
 size_t cn,mn;void *c=read_blob(argv[1],&cn),*m=read_blob(argv[2],&mn);
 size_t wall_size=0;
 unsigned char *wall_set=argc==5&&!apply_support?read_blob(argv[4],&wall_size):NULL;
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
   int reported_apex=0,reported_unsupported_descent=0,reported_source_sweep=0;
   SA1Vec3 reset_from={0};
   for(tick=0;tick<240;tick++){
    if(sa3ds_ec_residency_needs_recenter(&col,s.pos))
     sa3ds_ec_residency_recenter(&col,c,cn,roundf(s.pos.x/256)*256,roundf(s.pos.z/256)*256,&s);
    SA1Input in={0};SA3DSSpringPendingCommand cmd={0};
    sa3ds_spring_player_begin(&s,&cmd,&in);
    SA1PlayerState before=s;
    int was_grounded=s.grounded;
    SA1Vec3 previous_task=sa3ds_ec_task_origin(&s);
    uint32_t previous_instance=s.surface_id>=0&&(uint32_t)s.surface_id<col.triangle_count ?
        col.triangles[s.surface_id].source_instance : UINT32_MAX;
    SA1Vec3 previous_up=s.ground_normal, previous_forward=s.surface_forward;
    SA1Vec3 prev=s.pos;col.reference_y=s.pos.y;
    sa1_physics_step_steam_sonic_flat(&s,&in,0,&col.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
    if(s.sim_frame==0){reset_from=prev;break;}
    sa3ds_spring_player_tail(&s);
    if(chain&&was_grounded&&(task_angles||(!s.grounded&&(apply_support||!reported_source_sweep)))){
     SA1Contact end_support=sa3ds_ec_collision_contact(&col,s.pos,
         sa1_vec3_scale(previous_up,-1),2.35f);
     printf("\n  support probe tick=%u valid=%d",tick,end_support.valid);
     if(end_support.valid)
      printf(" distance=%.4f alignment=%.4f",end_support.signed_distance,
          sa1_vec3_dot(previous_up,end_support.normal));
     /* The point solver can snap the center and remove normal speed even
        on a successful tick. Use the unchanged movement before that snap. */
     SA1Vec3 motion=task_angles?s.pc2004_world_motion:sa1_vec3_sub(s.pos,prev);
     SA1Vec3 end=task_angles?sa1_vec3_add(prev,motion):s.pos;
     SA1Contact confirmed=report_source_sweep(
         &col,tick,previous_task,previous_up,motion,end,previous_instance,previous_forward);
     if(apply_support&&confirmed.valid){
      apply_confirmed_support(&s,&before,&confirmed,task_angles);
      const SA3DSECTriangle *face=&col.triangles[confirmed.surface_id];
      printf("\n  applied static support tick=%u face=%u/%u",
          tick,face->source_instance,face->source_triangle);
     }
     reported_source_sweep=1;
    }
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
    if(wall_set)wall_sweep(wall_set+32,wall_count,previous_task,&s,tick);
    int descending=!s.grounded&&s.vel.y<=0;
    int apex=descending&&!reported_apex;
    if(s.grounded)reported_apex=reported_unsupported_descent=0;
    if(was_grounded!=s.grounded||(descending&&!reported_unsupported_descent)){
     float floor_y;
     int resident=sa3ds_ec_collision_floor_below(&col,s.pos.x,s.pos.y,s.pos.z,&floor_y);
     /* Match the original floor helper's resident no-hit sentinel. */
     int unsupported_descent=descending&&(!resident||floor_y==-1000000.f);
     if(was_grounded!=s.grounded||apex||unsupported_descent){
      printf("\n  tick=%u grounded=%d pos=%.2f,%.2f,%.2f vel=%.2f,%.2f,%.2f flags=%08x",
       tick,s.grounded,s.pos.x,s.pos.y,s.pos.z,s.vel.x,s.vel.y,s.vel.z,s.surface_flags);
      if(!resident)printf(" floorBelow=outside-query-region");
      else if(floor_y==-1000000.f)printf(" floorBelow=none");
      else printf(" floorBelow=%.2f",floor_y);
      if(apex){printf(" firstNonascendingAirTick");reported_apex=1;}
      if(unsupported_descent){
       printf(" firstUnsupportedDescent");reported_unsupported_descent=1;
      }
     }
    }
    lowest=fminf(lowest,s.pos.y);air+=!s.grounded;
   }
   printf("\n  outcome=%s ticks=%u air=%u minY=%.2f grounded=%d speed=%.1f lock=%d end=%.1f,%.1f,%.1f",tick==240?"window-complete":"physics-reset",tick,air,lowest,s.grounded,params.speed,params.control_timer,s.pos.x,s.pos.y,s.pos.z);
   if(tick<240)printf(" resetFrom=%.2f,%.2f,%.2f",reset_from.x,reset_from.y,reset_from.z);
  }
  puts("");sa3ds_ec_collision_destroy(&col);
 }
 free(wall_set);free(c);free(m);return 0;
}
