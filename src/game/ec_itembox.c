#include "ec_itembox.h"
#include "sa3dspak.h"
#include <math.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)(p[0]|(p[1]<<8));}
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
int sa3ds_itembox_assets_open(SA3DSItemBoxAssets*out,const void*bytes,size_t size){
 if(!out)return 0;
 memset(out,0,sizeof(*out));SA3DSPak p;SA3DSPakChunk c;
 if(!bytes||sa3dspak_open(&p,bytes,size)!=SA3DSPAK_OK||sa3dspak_find(&p,"ITEMBOX",&c)!=SA3DSPAK_OK||c.flags||c.size<36+192)return 0;
 const uint8_t*b=c.data;SA3DSItemBoxAssets n={0};
 if(memcmp(b,"SA3IBOX\0",8)||r32(b+8)!=1)return 0;
 n.count=r32(b+12);uint32_t ro=r32(b+16);n.stride=r32(b+20);uint32_t cci=r32(b+24),total=r32(b+28);n.load_distance_squared=rf(b+32);
 if(total!=c.size||cci!=192||ro!=36+192||n.stride!=44||!n.count||n.count>256||ro+n.count*n.stride!=total||!(n.load_distance_squared>0))return 0;
 n.cci_intact=b+36;n.cci_broken=b+36+96;n.rows=b+ro;
 if(n.cci_intact[1]!=1||n.cci_intact[49]!=0)return 0;
 for(uint32_t i=0;i<n.count;i++){SA3DSItemBoxPlacement pl;if(!sa3ds_itembox_placement(&n,i,&pl))return 0;}
 *out=n;return 1;
}
int sa3ds_itembox_placement(const SA3DSItemBoxAssets*a,uint32_t i,SA3DSItemBoxPlacement*o){
 if(!a||!a->rows||!o||i>=a->count)return 0;
 const uint8_t*r=a->rows+(size_t)i*a->stride;SA3DSItemBoxPlacement p={0};
 p.act=r32(r);p.source_row=r32(r+4);p.type=r16(r+8);p.kind=r[10];
 for(unsigned k=0;k<3;k++){p.angles[k]=(int16_t)r16(r+12+k*2);p.position[k]=rf(r+20+k*4);p.scale[k]=rf(r+32+k*4);}
 if((p.type&4095)!=13||p.act>2||p.kind>8)return 0;
 for(unsigned k=0;k<3;k++)if(!isfinite(p.position[k])||!isfinite(p.scale[k]))return 0;
 *o=p;return 1;
}
static SA3DSItemBoxEntry*entry_of(SA3DSOwnedTaskList*l,uint32_t id){SA3DSOwnedTask*t=sa3ds_owned_task(l,id);return t?t->data:NULL;}
static int free_box(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 SA3DSItemBoxEntry*e=entry_of(l,id);if(!e)return 0;
 if(!sa3ds_owned_task_release(l,id,context))return 0;
 uint8_t freed;if(!sa3ds_set_task_run_free(&e->lifetime,id,1,NULL,NULL,&freed)||!freed)return 0;
 e->action=e->queued=e->broken_shapes=0;e->owner->frees++;return 1;
}
/*4D6E80*/
static void break_box(SA3DSItemBoxRuntime*r,SA3DSItemBoxEntry*e,const SA3DSItemBoxPlacement*p){
 e->lifetime.flags|=4;                                      /*46C0F0*/
 if(r->services.effect){float at[3]={p->position[0],(float)((double)p->position[1]+5.),p->position[2]};r->services.effect(r->services.context,at);}
 e->broken_shapes=1;e->action=4;r->breaks++;
}
static int main_box(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 (void)context;SA3DSItemBoxEntry*e=entry_of(l,id);if(!e)return 0;SA3DSItemBoxRuntime*r=e->owner;
 SA3DSItemBoxFrame f;SA3DSItemBoxPlacement p;e->queued=0;
 if(!r->services.read_frame(r->services.context,&f)||!sa3ds_itembox_placement(r->assets,e->row,&p))return 0;
 SA3DSSetClipInput clip=f.clip;clip.has_set_data=1;clip.set_flags=(uint8_t)e->lifetime.flags;
 clip.unload_distance_squared=e->lifetime.unload_distance_squared;memcpy(clip.position,p.position,12);
 uint8_t unload;if(!sa3ds_set_object_clip(&clip,&unload))return 0;          /*4D6F1C 46C360*/
 if(unload)return sa3ds_set_task_defer_free(&e->lifetime,id)&&sa3ds_owned_task_set_main(l,id,free_box);
 switch(e->action){
 case 0:
  e->lifetime.flags|=0x10;                                  /*46C100*/
  e->broken_shapes=0;e->action=1;e->grow=0;r->initializations++;
  if(e->lifetime.flags&4){e->broken_shapes=1;e->action=4;}  /*46C110 -> 3 -> 4D6B20 display*/
  break;
 case 1:{
  float g=(float)((double)e->grow+(double).11f);e->grow=g;
  if(g>=5.f){e->action=2;e->grow=1.f;}
  break;}
 case 2:{
  SA3DSItemBoxContact c={0,-1,-1,1};
  if(r->services.contact&&!r->services.contact(r->services.context,e->row,&c))return 0;
  int broke=0;
  if((c.status&1)&&(c.player==0||c.player==1)&&c.self_shape==1){        /*+A0==1: top sphere*/
   broke=1;if(!c.player_grounded&&r->services.bounce)r->services.bounce(r->services.context,c.player);
  }
  if(c.status&4)broke=1;                                     /*attacked*/
  if(broke){
   SA3DSItemAward a;
   if(!sa3ds_item_box_award(p.kind,0,r->services.rand15,r->services.context,&a))return 0;
   if(r->services.award)r->services.award(r->services.context,e->row,&a);
   r->awards++;e->action=3;
  }
  e->queued=1;                                               /*4D67ED: 41C280 always*/
  e->icon_angle=(float)((double)e->icon_angle+3.);
  break;}
 case 3:break_box(r,e,&p);break;                            /*no enqueue this update*/
 case 4:case 5:e->queued=1;break;
 default:e->action=0;break;
 }
 return 1;
}
int sa3ds_itemboxes_init(SA3DSItemBoxRuntime*r,const SA3DSItemBoxAssets*a,SA3DSOwnedTaskList*l,SA3DSItemBoxEntry*entries,
 const uint16_t*flags,uint32_t capacity,const SA3DSItemBoxServices*s){
 if(!r||!a||!a->rows||!l||!l->slots||!entries||!flags||capacity<a->count||!s||!s->read_frame)return 0;
 memset(r,0,sizeof(*r));r->assets=a;r->tasks=l;r->entries=entries;r->services=*s;
 memset(entries,0,(size_t)a->count*sizeof(*entries));
 for(uint32_t i=0;i<a->count;i++){entries[i].owner=r;entries[i].row=i;entries[i].lifetime.flags=flags[i];}
 return 1;
}
int sa3ds_itemboxes_load(SA3DSItemBoxRuntime*r){
 if(!r||!r->assets||!r->services.read_frame)return 0;
 SA3DSItemBoxFrame f;if(!r->services.read_frame(r->services.context,&f)||f.clip_level>2)return 0;
 const SA3DSSetClipReference*reference=f.clip.player0.present?&f.clip.player0:&f.clip.camera;
 if(!reference->present)return 1;
 for(uint32_t i=0;i<r->assets->count;i++){
  SA3DSItemBoxEntry*e=&r->entries[i];SA3DSItemBoxPlacement p;
  if(!sa3ds_itembox_placement(r->assets,i,&p))return 0;
  if(p.act!=f.act||!sa3ds_set_task_load_eligible(&e->lifetime))continue;
  unsigned high=(p.type>>12)&7;
  if(high&&(high==1?f.clip_level>=1:f.clip_level>=2))continue;
  int inside;if(!sa3ds_set_position_in_range(p.position,reference->position,r->assets->load_distance_squared,&inside))return 0;
  if(!inside)continue;
  uint32_t id=sa3ds_owned_task_allocate(r->tasks,main_box,e);
  if(!id){r->allocation_failures++;continue;}
  if(!sa3ds_set_task_load_commit(&e->lifetime,id,r->assets->load_distance_squared))return 0;
 }
 return 1;
}
