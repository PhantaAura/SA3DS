/*Owner-package gate for C CUBE SET lifetime: real rows, load at360000,
 *action0 init then per-update enqueue, 46C360 clip at load+100.*/
#include "ec_ccube.h"
#include "set_preparation.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {SA3DSCubeFrame frame;} Context;
static int read_frame(void*v,SA3DSCubeFrame*f){*f=((Context*)v)->frame;return 1;}
int main(int argc,char**argv){
 assert(argc==2||argc==3);int wall=argc==3&&!strcmp(argv[2],"--wall");FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);
 unsigned char*b=malloc((size_t)n);assert(fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);
 SA3DSCubeAssets a;assert((wall?sa3ds_wall_assets_open(&a,b,(size_t)n):sa3ds_cube_assets_open(&a,b,(size_t)n))&&a.count==(wall?117u:123u)&&a.load_distance_squared==360000.f);
 assert(a.registry_kind==(wall?20u:24u)&&a.category==(wall?8u:4u));
 SA3DSCubePlacement t;assert(sa3ds_cube_placement(&a,0,&t));float dims[3];sa3ds_cube_dimensions(&t,dims);
 assert(dims[0]==(float)(t.scale[0]+10.)&&dims[2]==(float)(t.scale[2]+10.));
 SA3DSOwnedTask slots[256];SA3DSOwnedTaskList list;assert(sa3ds_owned_tasks_init(&list,slots,256));
 static SA3DSCubeEntry entries[256];uint16_t types[256],flags[256];SA3DSSetTaskLifetime set[256];uint8_t current[256];
 for(uint32_t i=0;i<a.count;i++){SA3DSCubePlacement p;assert(sa3ds_cube_placement(&a,i,&p));types[i]=p.type;current[i]=p.act==t.act;}
 assert(sa3ds_set_cold_prepare(set,types,a.count,1,(uint8_t)t.act,0));
 for(unsigned i=0;i<a.count;i++)flags[i]=current[i]?set[i].flags:0;
 Context c={0};SA3DSCubeServices s={&c,read_frame};SA3DSCubeRuntime r;assert(sa3ds_cubes_init(&r,&a,&list,entries,flags,256,&s));
 c.frame.act=t.act;c.frame.clip.player0.present=c.frame.clip.camera.present=1;
 memcpy(c.frame.clip.player0.position,t.position,12);memcpy(c.frame.clip.camera.position,t.position,12);
 assert(sa3ds_cubes_load(&r)&&sa3ds_owned_tasks_run(&list,&c)&&r.initializations>=1&&!entries[0].queued&&entries[0].action==1);
 sa3ds_cubes_clear_queue(&r);
 assert(sa3ds_cubes_load(&r)&&sa3ds_owned_tasks_run(&list,&c)&&entries[0].queued);
 unsigned queued=0,last=UINT32_MAX;
 for(SA3DSCubeEntry *e=r.queue_head;e;e=e->queue_next){assert(e->queued&&e->row<last);last=e->row;queued++;assert(queued<=a.count);}
 assert(queued==r.initializations);
 unsigned loaded=r.initializations;
 /*Move far away: every loaded cube defers, then frees on the next traversal.*/
 c.frame.clip.player0.position[0]+=5000;c.frame.clip.camera.position[0]+=5000;
 sa3ds_cubes_clear_queue(&r);
 assert(sa3ds_owned_tasks_run(&list,&c)&&!entries[0].queued&&entries[0].lifetime.free_main);
 assert(sa3ds_owned_tasks_run(&list,&c)&&r.frees==loaded&&!entries[0].lifetime.task&&!list.head);
 printf("%s owner runtime: PASS %u rows, scale+10, %u loaded at row0, ordered queue, init then enqueue, deferred free\n",wall?"WALL":"C CUBE",a.count,loaded);
 free(b);
}
