/*Owner-package gate: real O PIER rows/quad drive DynamicCOL in a real resident
 *Emerald Coast collision region. Not a retail trajectory comparison.*/
#include "ec_pier.h"
#include "emerald_coast_collision.h"
#include "emerald_coast_residency.h"
#include "set_preparation.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {SA3DSPierFrame frame;SA3DSECDynamicSet*set;const SA3DSPierAssets*assets;unsigned removes,breaks,throws;uint32_t standing_task;float thrown[3];} Context;
static int read_frame(void*v,SA3DSPierFrame*f){*f=((Context*)v)->frame;return 1;}
static int add(void*v,uint32_t task,const SA3DSPierPlacement*p){
 Context*c=v;SA3CMatrix m;
 sa3c_matrix_build_object(&m,(SA3CVec3){p->position[0],p->position[1],p->position[2]},
  (SA3CVec3){0,(float)p->angles[1]*(6.2831853f/65536.f),0},(SA3CVec3){1,1,1},0);
 return sa3ds_ec_dynamic_add(c->set,task,c->assets->surface_flags,(const float(*)[3])c->assets->points,
  c->assets->point_count,c->assets->indices,c->assets->index_count,&m);
}
static void removed(void*v,uint32_t task){Context*c=v;c->removes++;assert(sa3ds_ec_dynamic_remove(c->set,task));}
static void broken(void*v,uint32_t task,const SA3DSPierPlacement*p){Context*c=v;(void)p;c->breaks++;if(!c->standing_task)c->standing_task=task;}
static int standing(void*v,uint32_t task){return ((Context*)v)->standing_task==task;}
static void thrown(void*v,const float p[3]){Context*c=v;c->throws++;memcpy(c->thrown,p,12);}
static uint16_t rnd(void*c){(void)c;static uint32_t r=1;r=r*214013u+2531011u;return (uint16_t)((r>>16)&32767u);}
static double sine(void*c,uint16_t a){(void)c;if(!(a&0x3fff)){const double v[]={0,1,0,-1};return v[a>>14];}return sin(a*6.2831853071795864769/65536.0);}
static unsigned char*load(const char*path,size_t*size){
 FILE*f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);
 unsigned char*b=malloc((size_t)n);assert(b&&fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);*size=(size_t)n;return b;
}
static int run(Context*c,SA3DSPierRuntime*r,SA3DSOwnedTaskList*l){return sa3ds_piers_load(r)&&sa3ds_owned_tasks_run(l,c);}
int main(int argc,char**argv){
 assert(argc==3);size_t ps,cs;unsigned char*pb=load(argv[1],&ps),*cb=load(argv[2],&cs);
 SA3DSPierAssets a;assert(sa3ds_pier_assets_open(&a,pb,ps)&&a.count==62&&a.point_count==4&&a.index_count==6);
 assert(a.surface_flags==0x68000001u&&a.break_distance_squared==5625.f&&a.clip_distance_squared==250000.f&&a.broken_ticks==300);
 SA3DSPierPlacement target;assert(sa3ds_pier_placement(&a,0,&target)&&target.act==0);
 SA3DSECCollision col;assert(sa3ds_ec_collision_open_region(&col,cb,cs,0,target.position[0],target.position[2]));
 static SA3DSECDynamicSet set;sa3ds_ec_dynamic_clear(&set);col.dynamic=&set;
 float before;assert(sa3ds_ec_collision_floor_below(&col,target.position[0],target.position[1]+20,target.position[2],&before));
 SA3DSOwnedTask slots[64];SA3DSOwnedTaskList list;assert(sa3ds_owned_tasks_init(&list,slots,64));
 Context c={.set=&set,.assets=&a};SA3DSPierServices s={&c,read_frame,add,removed,broken,standing,thrown,sine,rnd};
 SA3DSSetTaskLifetime prepared[62];uint16_t types[62],flags[62];
 for(uint32_t i=0;i<62;i++){SA3DSPierPlacement p;assert(sa3ds_pier_placement(&a,i,&p));types[i]=p.type;}
 assert(sa3ds_set_cold_prepare(prepared,types,62,1,0,0));for(unsigned i=0;i<62;i++)flags[i]=prepared[i].flags;
 SA3DSPierRuntime r;static SA3DSPierEntry entries[62];assert(sa3ds_piers_init(&r,&a,&list,entries,flags,62,&s));
 c.frame.clip.player0.present=c.frame.clip.camera.present=1;
 memcpy(c.frame.clip.player0.position,target.position,12);memcpy(c.frame.clip.camera.position,target.position,12);
 c.frame.whale_tip[1]=-100000;
 /*SET load allocates before the traversal, so constructors run this tick.*/
 assert(run(&c,&r,&list)&&r.constructs>=1&&set.owner_count==r.constructs);
 assert(run(&c,&r,&list)&&set.owner_count==r.constructs);
 unsigned loaded=r.constructs;
 assert(sa3ds_ec_collision_apply_dynamic(&col,NULL));
 float on;SA1Vec3 n;assert(sa3ds_ec_collision_floor_below_frame(&col,target.position[0],target.position[1]+20,target.position[2],&on,&n));
 assert(on>before+1&&fabsf(on-(target.position[1]+1.5f))<0.1f&&n.y>0.99f);
 /*Standing player keeps the dynamic support through a residency rebuild.*/
 SA1PlayerState p={0};p.pos=(SA1Vec3){target.position[0]+30,on+1,target.position[2]+30};p.grounded=1;
 col.reference_y=on+1;SA1Surface sf=sa3ds_ec_collision_surface(&col,target.position[0],target.position[2]);
 assert(sf.valid&&(uint32_t)sf.surface_id>=col.static_triangle_count);p.surface_id=sf.surface_id;
 uint32_t source=col.triangles[sf.surface_id].source_instance;
 assert(sa3ds_ec_residency_recenter(&col,cb,cs,target.position[0]+30,target.position[2]+30,&p));
 assert(p.surface_id>=0&&col.triangles[p.surface_id].source_instance==source&&col.dynamic==&set);
 /*Whale tip within75 units: break, display off, timer300, then deferred free.*/
 memcpy(c.frame.whale_tip,target.position,12);c.frame.whale_tip[1]+=74.f;
 c.frame.whale_movement[0]=-3.f;c.frame.whale_movement[1]=1.f;
 assert(run(&c,&r,&list)&&r.breaks>=1&&c.breaks==r.breaks);
 /*Only the first broken pier reports Sonic standing: one throw, speed8 along
  *the pier axis, upward2, oriented with the whale's movement.*/
 assert(c.throws==1&&r.throws==1&&c.thrown[1]==2.f&&fabsf(hypotf(c.thrown[0],c.thrown[2])-8.f)<1e-4f);
 assert(c.thrown[0]*c.frame.whale_movement[0]+c.thrown[2]*c.frame.whale_movement[1]>=0);
 unsigned breaks=r.breaks,ticks=0;c.frame.whale_tip[1]=-100000;
 SA3DSPierEntry*first=NULL;for(unsigned i=0;i<62;i++)if(entries[i].action==1){first=&entries[i];break;}
 assert(first&&first->piece_count==a.piece_count&&a.piece_count==25&&!first->pieces[0].constructed);
 assert(fabsf(hypotf(first->spread[0],first->spread[1]))>0&&fabsf(hypotf(first->spread[0],first->spread[1]))<=4.1f+1e-4f);
 unsigned splashed=0,hidden=0;
 while(r.frees<breaks&&ticks<400){
  assert(run(&c,&r,&list));ticks++;
  if(ticks==1)for(unsigned i=0;i<25;i++)assert(first->pieces[i].constructed&&first->pieces[i].action>=2&&first->pieces[i].velocity[1]>=2.5f);
  if(ticks==299)for(unsigned i=0;i<25;i++){const SA3DSPierPiece*q=&first->pieces[i];
   assert(isfinite(q->position[0])&&isfinite(q->position[1])&&isfinite(q->position[2]));splashed+=q->action>=5;hidden+=q->action==6;}
 }
 assert(splashed==25&&hidden>0);
 assert(r.frees==breaks&&ticks==302&&c.removes==breaks);
 assert(sa3ds_ec_collision_apply_dynamic(&col,NULL));
 float after;assert(sa3ds_ec_collision_floor_below(&col,target.position[0],target.position[1]+20,target.position[2],&after)&&fabsf(after-before)<0.001f);
 /*Freed SET row (bit0 cleared, 0x8000 kept) is eligible and reloads intact.*/
 assert(run(&c,&r,&list)&&r.constructs>loaded);
 printf("O PIER owner runtime: PASS 62 rows, quad y+1.5 DynamicCOL in real region (%u loaded), residency keeps dynamic support,"
  " tip<75 break + standing throw, 25 pieces construct/fly/splash/hide, 302-tick broken lifetime then free/remove, eligible reload; sound separate\n",loaded);
 sa3ds_ec_collision_destroy(&col);free(pb);free(cb);
}
