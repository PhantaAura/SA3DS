/* Isolate held-input43C130, using original incoming task state.
 * This does not compare terrain response, steering forces or a complete tick. */
#include "automatic_loop.h"
#include "pc2004_task_frame.h"
#include "steam_player_parameters.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int main(int argc,char **argv){
 if(argc!=2)return 2;
 FILE *file=fopen(argv[1],"rb");if(!file)return 2;
 if(fseek(file,0,SEEK_END))return 2;
 long n=ftell(file);if(n<=0)return 2;rewind(file);
 void *bytes=malloc((size_t)n);if(!bytes||fread(bytes,1,(size_t)n,file)!=(size_t)n)return 2;fclose(file);
 static SA3DSAutomaticLoops loops;
 if(!sa3ds_automatic_loops_open(&loops,bytes,(size_t)n))return 2;
 const SA1PC2004Loop *path=NULL;
 for(unsigned i=0;i<loops.count;i++)if(loops.actors[i].act==0&&loops.actors[i].ordinal==0)path=&loops.actors[i].path;
 if(!path)return 2;
 unsigned frame,x,y,z,motion_yaw,expected_yaw,checked=0,worst=0,failed=0;
 float distance,speed,expected_distance;SA1Vec3 origin,motion,up;
 float worst_distance=0;
 int got;
 while((got=scanf("%u %f %f %f %f %f %f %f %f %f %f %f %u %u %u %u %f %u",
  &frame,&distance,&speed,&origin.x,&origin.y,&origin.z,&motion.x,&motion.y,&motion.z,
  &up.x,&up.y,&up.z,&x,&y,&z,&motion_yaw,&expected_distance,&expected_yaw))==18){
  if(motion_yaw!=y){fprintf(stderr,"Unsupported motion/task heading difference at %u\n",frame);return 2;}
  SA1SurfaceFrame f;if(!sa1_pc2004_task_frame_from_angles(&f,(uint16_t)x,(uint16_t)y,(uint16_t)z,NULL))return 2;
  /* Original csts Up is an input, independent of terrain matching. */
  f.up=up;
  SA1PlayerState p={.pos=sa1_vec3_add(origin,f.up),.pc2004_loop=path,
    .pc2004_loop_distance=distance,.pc2004_status_flags=0x2000};
  int active=sa1_pc2004_loop_steer(&p,&f,&speed,SA1_STEAM_SONIC_PARAMETER_EVIDENCE.dash_speed,motion);
  uint16_t yaw;if(!sa1_pc2004_task_yaw(&f,&yaw))return 2;
  int difference=(int16_t)(uint16_t)(yaw-expected_yaw);unsigned error=(unsigned)abs(difference);
  if(error>worst)worst=error;
  float distance_error=fabsf(p.pc2004_loop_distance-expected_distance);
  if(distance_error>worst_distance)worst_distance=distance_error;
  checked++;
  if(!active||distance_error!=0.f||error>1){
   if(!failed)fprintf(stderr,"First divergence frame%u: active%d distance%.9g expected%.9g yaw%u expected%u\n",
      frame,active,p.pc2004_loop_distance,expected_distance,yaw,expected_yaw);
   failed++;
  }
 }
 free(bytes);
 if(got!=EOF||!checked)return 2;
 printf("43C130 component: %u checked, %u failed, maximum heading error %u BAMS, distance error %.9g\n",checked,failed,worst,worst_distance);
 return failed?1:0;
}
