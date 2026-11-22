/* Replay only the source category8 response between recorded Sonic updates.
 * Physics/animation come from the external trace, not from this program. */
#include "ec_ccube.h"
#include "ccl_wall.h"
#include "set_preparation.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
typedef struct {SA3DSCubeFrame frame;const unsigned char *lut;} Context;
static int read_frame(void *p,SA3DSCubeFrame *f){*f=((Context*)p)->frame;return 1;}
static float raw(const unsigned char *p){float f;memcpy(&f,p,4);return f;}
static double sample(Context *c,unsigned i){
 i&=4095;
 if(i<1024)return raw(c->lut+i*4);
 if(i<2048)return raw(c->lut+(2048-i)*4);
 if(i<3072)return -raw(c->lut+(i-2048)*4);
 return -raw(c->lut+(4096-i)*4);
}
static double sine(void *v,uint16_t angle){
 Context *c=v;unsigned i=angle>>4,frac=angle&15;double a=sample(c,i);
 return frac?a+(sample(c,i+1)-a)*(frac*.0625):a;
}
static void *blob(const char *name,size_t *size){
 FILE *f=fopen(name,"rb");if(!f)exit(2);if(fseek(f,0,SEEK_END))exit(2);
 long n=ftell(f);if(n<=0)exit(2);*size=(size_t)n;rewind(f);
 void *p=malloc(*size);if(!p||fread(p,1,*size,f)!=*size)exit(2);fclose(f);return p;
}
int main(int argc,char **argv){
 if(argc!=3&&argc!=4){fprintf(stderr,"usage: check-walls owner_wall_package owner_quarter_lut [immediate-load]\n");return 2;}
 size_t n,ln;void *bytes=blob(argv[1],&n);unsigned char *lut=blob(argv[2],&ln);if(ln!=4100)return 2;
 SA3DSCubeAssets a;if(!sa3ds_wall_assets_open(&a,bytes,n)||a.count>256)return 2;
 static SA3DSCubeEntry entries[256];static SA3DSOwnedTask slots[256];SA3DSOwnedTaskList tasks;
 if(!sa3ds_owned_tasks_init(&tasks,slots,256))return 2;
 SA3DSCubeRuntime walls;Context context={.lut=lut};SA3DSCubeServices services={&context,read_frame};
 uint16_t flags[256];for(unsigned i=0;i<a.count;i++)flags[i]=0x8000;
 if(!sa3ds_cubes_init(&walls,&a,&tasks,entries,flags,256,&services))return 2;
 context.frame.act=0;context.frame.clip.player0.present=context.frame.clip.camera.present=1;
 unsigned frame,checked=0,failed=0,soft=0,hard=0;float speed;SA1Vec3 task,expected,history={0};
 float worst=0;int got;
 while((got=scanf("%u %f %f %f %f %f %f %f",&frame,&task.x,&task.y,&task.z,
        &expected.x,&expected.y,&expected.z,&speed))==8){
  if(frame==0)history=task;
  memcpy(context.frame.clip.player0.position,&task,12);memcpy(context.frame.clip.camera.position,&task,12);
  sa3ds_cubes_clear_queue(&walls);
  if(argc==4&&!sa3ds_cubes_load(&walls))return 2;
  if(!sa3ds_owned_tasks_run(&tasks,&context))return 2;
  if(argc==3&&!sa3ds_cubes_load(&walls))return 2;
  SA1Vec3 current=task,remember=task;
  for(SA3DSCubeEntry *e=walls.queue_head;e;e=e->queue_next){
   SA3DSCubePlacement p;if(!sa3ds_cube_placement(&a,e->row,&p))return 2;
   float d[3];sa3ds_cube_dimensions(&p,d);SA1Vec3 dims={d[0],d[1],d[2]};
   SA1Vec3 center={p.position[0],p.position[1],p.position[2]};
   double dx=(double)current.x-center.x,dy=(double)current.y-center.y,dz=(double)current.z-center.z;
   double broad=(double)speed+10.+sa3ds_ccl_wall_broad_radius(dims);
   if(dx*dx+dy*dy+dz*dz>=broad*broad)continue;
   SA1Vec3 next;SA3DSWallResponse response=sa3ds_ccl_wall_world_sweep(history,current,center,(uint16_t)p.angles[1],4,4,dims,sine,&context,&next);
   if(response==SA3DS_WALL_MISS)continue;
   current=next;
   if(response==SA3DS_WALL_HARD){remember=current;hard++;}else soft++;
  }
  history=remember;
  float error=fmaxf(fabsf(current.x-expected.x),fmaxf(fabsf(current.y-expected.y),fabsf(current.z-expected.z)));
  if(error>worst)worst=error;
  checked++;
  if(error>.001f){
   if(failed<6)fprintf(stderr,"frame%u got%.6f,%.6f expected%.6f,%.6f error%.6f\n",frame,current.x,current.z,expected.x,expected.z,error);
   failed++;
  }
 }
 printf("WALL inter-update replay: %u checked %u failed maxError%.9g soft%u hard%u\n",checked,failed,worst,soft,hard);
 free(bytes);free(lut);return got!=EOF||!checked?2:failed?1:0;
}
