#include "ec_ccube.h"
#include "sa3dspak.h"
#include <math.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)(p[0]|(p[1]<<8));}
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
int sa3ds_cube_assets_open(SA3DSCubeAssets*out,const void*bytes,size_t size){
 if(!out)return 0;
 memset(out,0,sizeof(*out));SA3DSPak p;SA3DSPakChunk c;
 if(!bytes||sa3dspak_open(&p,bytes,size)!=SA3DSPAK_OK||sa3dspak_find(&p,"CCUBE",&c)!=SA3DSPAK_OK||c.flags||c.size<84)return 0;
 const uint8_t*b=c.data;SA3DSCubeAssets n={0};
 if(memcmp(b,"SA3CUBE\0",8)||r32(b+8)!=1||r32(b+24)!=4)return 0;
 n.count=r32(b+12);uint32_t ro=r32(b+16);n.stride=r32(b+20);uint32_t total=r32(b+28);n.load_distance_squared=rf(b+32);
 if(total!=c.size||ro!=84||n.stride!=42||!n.count||n.count>1024||ro+n.count*n.stride!=total||!(n.load_distance_squared>0)||b[37]!=3)return 0;
 n.cci=b+36;n.rows=b+ro;
 for(uint32_t i=0;i<n.count;i++){SA3DSCubePlacement pl;if(!sa3ds_cube_placement(&n,i,&pl))return 0;}
 *out=n;return 1;
}
int sa3ds_cube_placement(const SA3DSCubeAssets*a,uint32_t i,SA3DSCubePlacement*o){
 if(!a||!a->rows||!o||i>=a->count)return 0;
 const uint8_t*r=a->rows+(size_t)i*a->stride;SA3DSCubePlacement p={0};
 p.act=r32(r);p.source_row=r32(r+4);p.type=r16(r+8);
 for(unsigned k=0;k<3;k++){p.angles[k]=(int16_t)r16(r+10+k*2);p.position[k]=rf(r+18+k*4);p.scale[k]=rf(r+30+k*4);}
 if((p.type&4095)!=24||p.act>2)return 0;
 for(unsigned k=0;k<3;k++)if(!isfinite(p.position[k])||!isfinite(p.scale[k]))return 0;
 *o=p;return 1;
}
void sa3ds_cube_dimensions(const SA3DSCubePlacement*p,float out[3]){for(unsigned k=0;k<3;k++)out[k]=(float)((double)p->scale[k]+10.);}
static SA3DSCubeEntry*entry_of(SA3DSOwnedTaskList*l,uint32_t id){SA3DSOwnedTask*t=sa3ds_owned_task(l,id);return t?t->data:NULL;}
static int free_cube(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 SA3DSCubeEntry*e=entry_of(l,id);if(!e)return 0;
 if(!sa3ds_owned_task_release(l,id,context))return 0;
 uint8_t freed;if(!sa3ds_set_task_run_free(&e->lifetime,id,1,NULL,NULL,&freed)||!freed)return 0;
 e->action=e->queued=0;e->owner->frees++;return 1;
}
static int main_cube(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 (void)context;SA3DSCubeEntry*e=entry_of(l,id);if(!e)return 0;SA3DSCubeRuntime*r=e->owner;
 SA3DSCubeFrame f;SA3DSCubePlacement p;e->queued=0;
 if(!r->services.read_frame(r->services.context,&f)||!sa3ds_cube_placement(r->assets,e->row,&p))return 0;
 /*4D4802: 46C360 uses runtime SET+0xC (load+100).*/
 SA3DSSetClipInput clip=f.clip;clip.has_set_data=1;clip.set_flags=(uint8_t)e->lifetime.flags;
 clip.unload_distance_squared=e->lifetime.unload_distance_squared;memcpy(clip.position,p.position,12);
 uint8_t unload;if(!sa3ds_set_object_clip(&clip,&unload))return 0;
 if(unload)return sa3ds_set_task_defer_free(&e->lifetime,id)&&sa3ds_owned_task_set_main(l,id,free_cube);
 if(e->action==0){e->action=1;r->initializations++;}  /*41CAF0+4D46C0, no enqueue this update*/
 else if(e->action==1)e->queued=1;                     /*41C280*/
 else e->action=0;
 return 1;
}
int sa3ds_cubes_init(SA3DSCubeRuntime*r,const SA3DSCubeAssets*a,SA3DSOwnedTaskList*l,SA3DSCubeEntry*entries,
 const uint16_t*flags,uint32_t capacity,const SA3DSCubeServices*s){
 if(!r||!a||!a->rows||!l||!l->slots||!entries||!flags||capacity<a->count||!s||!s->read_frame)return 0;
 memset(r,0,sizeof(*r));r->assets=a;r->tasks=l;r->entries=entries;r->services=*s;
 memset(entries,0,(size_t)a->count*sizeof(*entries));
 for(uint32_t i=0;i<a->count;i++){entries[i].owner=r;entries[i].row=i;entries[i].lifetime.flags=flags[i];}
 return 1;
}
int sa3ds_cubes_load(SA3DSCubeRuntime*r){
 if(!r||!r->assets||!r->services.read_frame)return 0;
 SA3DSCubeFrame f;if(!r->services.read_frame(r->services.context,&f)||f.clip_level>2)return 0;
 const SA3DSSetClipReference*reference=f.clip.player0.present?&f.clip.player0:&f.clip.camera;
 if(!reference->present)return 1;
 for(uint32_t i=0;i<r->assets->count;i++){
  SA3DSCubeEntry*e=&r->entries[i];SA3DSCubePlacement p;
  if(!sa3ds_cube_placement(r->assets,i,&p))return 0;
  if(p.act!=f.act||!sa3ds_set_task_load_eligible(&e->lifetime))continue;
  if(e->lifetime.flags&2)return 0;
  unsigned high=(p.type>>12)&7;
  if(high&&(high==1?f.clip_level>=1:f.clip_level>=2))continue;
  int inside;if(!sa3ds_set_position_in_range(p.position,reference->position,r->assets->load_distance_squared,&inside))return 0;
  if(!inside)continue;
  uint32_t id=sa3ds_owned_task_allocate(r->tasks,main_cube,e); /*registry callback is the main*/
  if(!id){r->allocation_failures++;continue;}
  if(!sa3ds_set_task_load_commit(&e->lifetime,id,r->assets->load_distance_squared))return 0;
 }
 return 1;
}
