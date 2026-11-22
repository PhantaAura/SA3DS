/* First EC loop static-contact component comparator. Every update receives
 * original movement and incoming support identity. This is not a native solver,
 * full broad phase, wall response, or an integrated port trajectory. */
#include "emerald_coast_collision.h"
#include "pc2004_sphere_sweep.h"
#include "pc2004_collision_work.h"
#include "surface_frame.h"
#include "pc2004_task_frame.h"
#include "steam_player_parameters.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
static SA1Vec3 loop_center(SA1Vec3 origin,SA1Vec3 up,float height,float vertical,float error){
 (void)vertical;float scale=up.y<=-.6f?1.8f:up.y>.05f?1.4f:1.65f;
 height=(float)((double)height-(double)error*scale);
 return (SA1Vec3){origin.x+up.x*height,origin.y+up.y*height,origin.z+up.z*height};
}
static SA1Vec3 vec(const float a[3]){return (SA1Vec3){a[0],a[1],a[2]};}
static SA3DSSweepObject sweep(SA3DSECCollision *c,unsigned begin,unsigned end,SA1Vec3 center,SA1Vec3 motion,float radius){
 SA3DSSweepObject o={0};float s[]={center.x,center.y,center.z},d[]={motion.x,motion.y,motion.z};
 for(unsigned i=begin;i<end&&o.count<16;i++){
  SA3DSECTriangle *t=&c->triangles[i];if(t->flags&0x400002)continue;
  SA3DSSweepTriangle tr={.v={{t->a.x,t->a.y,t->a.z},{t->b.x,t->b.y,t->b.z},{t->c.x,t->c.y,t->c.z}},.flags=t->flags};
  SA3DSSweepHit h;if(sa3ds_pc2004_sweep_triangle(&tr,s,d,radius,&h))sa3ds_pc2004_sweep_record(&o,&h);
 }
 return o;
}
int main(int argc,char **argv){
 if(argc!=2)return 2;FILE*f=fopen(argv[1],"rb");if(!f)return 2;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);void*b=malloc(n);if(!b||fread(b,1,n,f)!=(size_t)n)return 2;fclose(f);
 SA3DSECCollision c={0};int opened=0;unsigned checked=0,failed=0;float worst=0,worst_normal=0;
 unsigned tick,previous;float forward;SA1Vec3 task,up,motion,expected,normal;int got_fields;
 while((got_fields=scanf("%u %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %u %f",&tick,&task.x,&task.y,&task.z,&up.x,&up.y,&up.z,&motion.x,&motion.y,&motion.z,&expected.x,&expected.y,&expected.z,&normal.x,&normal.y,&normal.z,&previous,&forward))==18){
  if(!opened){if(!sa3ds_ec_collision_open_region(&c,b,n,0,task.x,task.z))return 2;if(c.archive.format_minor!=1){fprintf(stderr,"Regenerate owner collision with source strip anchors\n");return 2;}opened=1;}
  float vertical=sa1_vec3_dot(motion,up);SA1Vec3 center=loop_center(task,up,5,vertical,2);

  unsigned beginlist[128],endlist[128],objects=0;
  for(unsigned begin=0;begin<c.triangle_count;){unsigned end=begin+1;while(end<c.triangle_count&&c.triangles[end].source_instance==c.triangles[begin].source_instance)end++;
   SA3DSSweepObject a=sweep(&c,begin,end,center,motion,5);
   if(a.count){if(objects==128){fprintf(stderr,"Object capacity exceeded\n");return 2;}beginlist[objects]=begin;endlist[objects++]=end;}
   begin=end;
  }
  float best=1000,err=1000;SA1Vec3 got=task,frameup=up,workcenter=center,workmotion=motion;
  unsigned order[128],on=0,last=128;
  for(unsigned i=objects;i>0;i--){unsigned index=i-1;if(c.triangles[beginlist[index]].source_instance==previous)last=index;else order[on++]=index;}
  if(last<128)order[on++]=last;
  unsigned accepted=0;
  for(unsigned item=0;item<on;item++){
   unsigned begin=beginlist[order[item]],end=endlist[order[item]];
   SA3DSSweepObject a=sweep(&c,begin,end,workcenter,workmotion,5);
   if(!a.count)continue;
   SA1Vec3 firstnormal=vec(a.selected.signed_normal);
   if(!sa1_pc2004_collision_ground_side(frameup,firstnormal,(SA1Vec3){0,-1,0},c.triangles[begin].flags))continue;
   SA1Vec3 avg=sa1_vec3_scale(vec(a.corrected_sum),1.f/a.count);
   SA1Vec3 moved=sa1_vec3_add(got,sa1_vec3_sub(avg,workcenter));
   SA1Vec3 confirm=loop_center(moved,frameup,4,vertical,2);
   float jog=SA1_STEAM_SONIC_PARAMETER_EVIDENCE.jog_speed,dash=SA1_STEAM_SONIC_PARAMETER_EVIDENCE.dash_speed;
   float radius=forward>=dash?5.1f:forward<jog?4.1f:(float)(((double)forward-jog)/((double)dash-jog)+(double)4.1f);
   SA3DSSweepObject o=sweep(&c,begin,end,confirm,workmotion,radius);
   if(!o.count)continue;
   SA1Vec3 norm=vec(o.selected.signed_normal);
   if(!sa1_pc2004_collision_ground_side(frameup,norm,(SA1Vec3){0,-1,0},c.triangles[begin].flags))continue;
   int32_t ax,az;sa3ds_pc2004_normal_angles(norm.x,norm.y,norm.z,&ax,&az);
   SA1SurfaceFrame f;sa1_pc2004_task_frame_from_angles(&f,(uint16_t)ax,0,(uint16_t)az,NULL);
   frameup=f.up;best=sa1_vec3_length(sa1_vec3_sub(norm,normal));
   got=sa1_pc2004_collision_support_position(sa1_vec3_scale(vec(o.corrected_sum),1.f/o.count),vec(o.selected.delta_xz),vec(o.selected.plane_point),frameup);
   workcenter=sa1_vec3_add(got,sa1_vec3_scale(frameup,5));workmotion=(SA1Vec3){0};accepted++;
  }
  if(accepted)err=sa1_vec3_length(sa1_vec3_sub(got,expected));

  checked++;if(err>worst)worst=err;if(best>worst_normal)worst_normal=best;
  if(!accepted||err>.003f||best>1e-5f){
   if(failed<6)fprintf(stderr,"frame%u objects%u accepted%u positionError%.9g normalError%.9g\n",tick,objects,accepted,err,best);
   failed++;
  }
 }
 sa3ds_ec_collision_destroy(&c);free(b);
 if(got_fields!=EOF||!checked)return 2;
 printf("First-loop static contact component: %u checked %u failed maxPosition%.9g maxNormal%.9g\n",checked,failed,worst,worst_normal);
 return failed?1:0;
}
