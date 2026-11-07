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
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);
 unsigned char*b=malloc((size_t)n);assert(fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);
 SA3DSCubeAssets a;assert(sa3ds_cube_assets_open(&a,b,(size_t)n)&&a.count==123&&a.load_distance_squared==360000.f);
 SA3DSCubePlacement t;assert(sa3ds_cube_placement(&a,0,&t));float dims[3];sa3ds_cube_dimensions(&t,dims);
 assert(dims[0]==(float)(t.scale[0]+10.)&&dims[2]==(float)(t.scale[2]+10.));
 SA3DSOwnedTask slots[256];SA3DSOwnedTaskList list;assert(sa3ds_owned_tasks_init(&list,slots,256));
 static SA3DSCubeEntry entries[123];uint16_t types[123],flags[123];SA3DSSetTaskLifetime set[123];uint8_t current[123];
 for(uint32_t i=0;i<123;i++){SA3DSCubePlacement p;assert(sa3ds_cube_placement(&a,i,&p));types[i]=p.type;current[i]=p.act==t.act;}
 assert(sa3ds_set_cold_prepare(set,types,123,1,(uint8_t)t.act,0));
 for(unsigned i=0;i<123;i++)flags[i]=current[i]?set[i].flags:0;
 Context c={0};SA3DSCubeServices s={&c,read_frame};SA3DSCubeRuntime r;assert(sa3ds_cubes_init(&r,&a,&list,entries,flags,123,&s));
 c.frame.act=t.act;c.frame.clip.player0.present=c.frame.clip.camera.present=1;
 memcpy(c.frame.clip.player0.position,t.position,12);memcpy(c.frame.clip.camera.position,t.position,12);
 assert(sa3ds_cubes_load(&r)&&sa3ds_owned_tasks_run(&list,&c)&&r.initializations>=1&&!entries[0].queued&&entries[0].action==1);
 assert(sa3ds_cubes_load(&r)&&sa3ds_owned_tasks_run(&list,&c)&&entries[0].queued);
 unsigned loaded=r.initializations;
 /*Move far away: every loaded cube defers, then frees on the next traversal.*/
 c.frame.clip.player0.position[0]+=5000;c.frame.clip.camera.position[0]+=5000;
 assert(sa3ds_owned_tasks_run(&list,&c)&&!entries[0].queued&&entries[0].lifetime.free_main);
 assert(sa3ds_owned_tasks_run(&list,&c)&&r.frees==loaded&&!entries[0].lifetime.task&&!list.head);
 printf("C CUBE owner runtime: PASS 123 rows, scale+10 dims, %u loaded at row0, init then enqueue, 46C360 deferred free; CCL push in native pass\n",loaded);
 free(b);
}
