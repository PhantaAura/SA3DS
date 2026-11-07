#include "ec_pier.h"
#include "sa3dspak.h"
#include <math.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)(p[0]|(p[1]<<8));}
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
int sa3ds_pier_assets_open(SA3DSPierAssets*out,const void*bytes,size_t size){
 if(!out)return 0;
 memset(out,0,sizeof(*out));SA3DSPak p;SA3DSPakChunk c;
 if(!bytes||sa3dspak_open(&p,bytes,size)!=SA3DSPAK_OK||sa3dspak_find(&p,"PIER",&c)!=SA3DSPAK_OK||c.flags||c.size<64)return 0;
 const uint8_t*b=c.data;SA3DSPierAssets n={0};
 if(memcmp(b,"SA3PIER\0",8)||r32(b+8)!=1)return 0;
 n.count=r32(b+12);uint32_t ro=r32(b+16);n.stride=r32(b+20);n.point_count=r32(b+24);uint32_t po=r32(b+28);
 n.index_count=r32(b+32);uint32_t io=r32(b+36),total=r32(b+40);n.surface_flags=r32(b+44);
 n.break_distance_squared=rf(b+48);n.clip_distance_squared=rf(b+52);n.load_distance_squared=rf(b+56);n.stand_radius=rf(b+60);
 n.broken_ticks=r32(b+64);
 if(total!=c.size||n.stride!=48||ro!=68||!n.count||n.count>1024||po!=ro+n.count*48||!n.point_count||n.point_count>SA3DS_PIER_MAX_POINTS||
    io!=po+n.point_count*12||!n.index_count||n.index_count%3||n.index_count>SA3DS_PIER_MAX_INDICES||io+n.index_count*2!=total||
    !(n.surface_flags&1)||!(n.break_distance_squared>0)||!(n.clip_distance_squared>0)||!(n.load_distance_squared>0)||!isfinite(n.stand_radius))return 0;
 for(uint32_t i=0;i<n.point_count;i++)for(unsigned k=0;k<3;k++){n.points[i][k]=rf(b+po+i*12+k*4);if(!isfinite(n.points[i][k]))return 0;}
 for(uint32_t i=0;i<n.index_count;i++){n.indices[i]=r16(b+io+i*2);if(n.indices[i]>=n.point_count)return 0;}
 n.rows=b+ro;
 SA3DSPakChunk k;if(sa3dspak_find(&p,"PIER_CCL",&k)!=SA3DSPAK_OK||k.flags||k.size!=192)return 0;
 for(unsigned i=0;i<4;i++)if(((const uint8_t*)k.data)[i*48+1]!=3)return 0;
 n.ccl=(const uint8_t*)k.data;
 SA3DSPakChunk q;if(sa3dspak_find(&p,"PIER_PIECES",&q)!=SA3DSPAK_OK||q.flags||q.size<16)return 0;
 const uint8_t*qb=q.data;
 if(memcmp(qb,"SA3PPCS\0",8)||r32(qb+8)!=1)return 0;
 n.piece_count=r32(qb+12);if(!n.piece_count||n.piece_count>SA3DS_PIER_MAX_PIECES||q.size!=16+n.piece_count*12)return 0;
 for(uint32_t i=0;i<n.piece_count;i++)for(unsigned j=0;j<3;j++){n.piece_offsets[i][j]=rf(qb+16+i*12+j*4);if(!isfinite(n.piece_offsets[i][j]))return 0;}
 for(uint32_t i=0;i<n.count;i++){SA3DSPierPlacement pl;if(!sa3ds_pier_placement(&n,i,&pl))return 0;}
 *out=n;return 1;
}
int sa3ds_pier_placement(const SA3DSPierAssets*a,uint32_t i,SA3DSPierPlacement*o){
 if(!a||!a->rows||!o||i>=a->count)return 0;
 const uint8_t*r=a->rows+(size_t)i*a->stride;SA3DSPierPlacement p={0};
 p.act=r32(r);p.source_row=r32(r+4);p.type=r16(r+8);p.variant=r[10];
 for(unsigned k=0;k<3;k++){p.angles[k]=(int16_t)r16(r+12+k*2);p.position[k]=rf(r+20+k*4);p.scale[k]=rf(r+32+k*4);}
 if((p.type&4095)!=59||p.variant>1||p.act>2)return 0;
 for(unsigned k=0;k<3;k++)if(!isfinite(p.position[k])||!isfinite(p.scale[k]))return 0;
 *o=p;return 1;
}
static double unit(SA3DSPierRuntime*r){return (double)r->services.random15(r->services.context)*(double)(1.f/32768.f);}
/*4FEBC0: delay/action from the first rand, then velocity and spin from five more.*/
static void construct_piece(SA3DSPierRuntime*r,const SA3DSPierEntry*e,SA3DSPierPiece*q){
 q->timer=0;q->action=2;
 double u=unit(r);
 if(u<(double).6f){q->timer=0;q->action=3;}
 else if(u<(double).75f)q->timer=2;
 else if(u<(double).95f)q->timer=3;
 else q->timer=5;
 q->velocity[0]=(float)(unit(r)*e->spread[0]*(double)1.8f);
 q->velocity[1]=(float)(unit(r)*(double)1.5f+(double)2.5f);
 q->velocity[2]=(float)(unit(r)*e->spread[1]*(double)1.8f);
 q->spin_y=(int32_t)((unit(r)-.5)*1800.);  /*ftol truncation*/
 q->spin_x=(int32_t)((unit(r)-.5)*3000.);
 q->constructed=1;q->displayed=1;
}
/*4FE9B0 per-update state machine; display follows actions2..5.*/
static void update_piece(SA3DSPierPiece*q){
 float*p=q->position,*v=q->velocity;
 switch(q->action){
 case 2:q->timer--;if(!q->timer){q->timer=0;q->action=3;}break; /*decw then unsigned ja*/
 case 3:case 4:
  p[0]=(float)((double)v[0]+p[0]);q->rot_y+=q->spin_y;q->rot_x+=q->spin_x;
  p[1]=(float)((double)v[1]+p[1]);p[2]=(float)((double)v[2]+p[2]);
  v[1]=(float)((double)v[1]-(double).1f);
  if(q->action==3){if(v[1]<0)q->action=4;}
  else if(p[1]<=-1.5f){q->action=5;q->timer=0;v[1]=-.7f;}  /*4FEAE9 compares new Y*/
  break;
 case 5:
  q->timer++;if(q->timer>120)q->action=6;
  p[0]=(float)((double)v[0]+p[0]);p[1]=(float)((double)v[1]+p[1]);p[2]=(float)((double)v[2]+p[2]);
  v[0]=(float)((double)v[0]*(double).93f);v[2]=(float)((double)v[2]*(double).93f);
  break;
 case 6:q->displayed=0;break;
 default:break;
 }
}
static SA3DSPierEntry*entry_of(SA3DSOwnedTaskList*l,uint32_t id){SA3DSOwnedTask*t=sa3ds_owned_task(l,id);return t?t->data:NULL;}
/*4FE970: child deletion (none in this profile), then43B380 when registered.*/
static int destruct_pier(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 (void)context;SA3DSPierEntry*e=entry_of(l,id);if(!e)return 0;SA3DSPierRuntime*r=e->owner;
 e->piece_count=0; /*4FE970 40B7E0 deletes children*/
 if(e->dynamic){r->services.dynamic_remove(r->services.context,id);e->dynamic=0;}
 e->displayed=0;return 1;
}
/*40B570 after either46C010 clip or the broken-state40B7E0 deletion.*/
static int free_pier(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 SA3DSPierEntry*e=entry_of(l,id);if(!e)return 0;SA3DSPierRuntime*r=e->owner;
 if(!sa3ds_owned_task_release(l,id,context))return 0;
 uint8_t freed;if(!sa3ds_set_task_run_free(&e->lifetime,id,1,NULL,NULL,&freed)||!freed)return 0;
 e->action=0;e->timer=0;r->frees++;return 1;
}
static int defer_free(SA3DSOwnedTaskList*l,uint32_t id,SA3DSPierEntry*e){
 return sa3ds_set_task_defer_free(&e->lifetime,id)&&sa3ds_owned_task_set_main(l,id,free_pier);
}
static int main_pier(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 (void)context;SA3DSPierEntry*e=entry_of(l,id);if(!e)return 0;SA3DSPierRuntime*r=e->owner;
 SA3DSPierFrame f;SA3DSPierPlacement p;
 if(!r->services.read_frame(r->services.context,&f)||!sa3ds_pier_placement(r->assets,e->row,&p))return 0;
 /*4FED3C: 46C010 with the explicit250000 threshold (not SET data+0xC).*/
 SA3DSSetClipInput clip=f.clip;clip.has_set_data=1;clip.set_flags=(uint8_t)e->lifetime.flags;
 clip.unload_distance_squared=r->assets->clip_distance_squared;memcpy(clip.position,p.position,12);
 uint8_t unload;if(!sa3ds_set_object_clip(&clip,&unload))return 0;
 if(unload)return defer_free(l,id,e);
 if(e->action==0){
  /*4FEDF2..4FEE51: squared tip distance below5625 breaks the pier.*/
  double d[3];for(unsigned k=0;k<3;k++)d[k]=(double)f.whale_tip[k]-p.position[k];
  if(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]<r->assets->break_distance_squared){
   e->action=1;e->timer=0;e->displayed=0;r->breaks++; /*4FEF39 clears the display callback*/
   /*4FEE57..4FEF36: spread from the unit perpendicular of whale movement
    *(pier side) and the unit tip->pier direction, both horizontal (418AC0).*/
   double dx=(float)((double)p.position[0]-f.whale_tip[0]),dz=(float)((double)p.position[2]-f.whale_tip[2]);
   double mx=f.whale_movement[0],mz=f.whale_movement[1],cross=dx*mz-dz*mx;
   double px=cross<0?-mz:mz,pz=cross<0?mx:-mx,pl=sqrt(px*px+pz*pz),dl=sqrt(dx*dx+dz*dz);
   float upx=pl==0?0:(float)(px/pl),upz=pl==0?0:(float)(pz/pl),udx=dl==0?0:(float)(dx/dl),udz=dl==0?0:(float)(dz/dl);
   e->spread[0]=(float)((double)upx*(double)2.9f+(double)udx*(double)1.2f);
   e->spread[1]=(float)((double)upz*(double)2.9f+(double)udz*(double)1.2f);
   /*4FEFD6: child pieces at pier pos + unrotated object offset, parent yaw.*/
   e->piece_count=r->services.random15?r->assets->piece_count:0;
   for(uint32_t i=0;i<e->piece_count;i++){
    SA3DSPierPiece*q=&e->pieces[i];memset(q,0,sizeof(*q));
    for(unsigned k=0;k<3;k++)q->position[k]=(float)((double)p.position[k]+r->assets->piece_offsets[i][k]);
    q->rot_y=p.angles[1];
   }
   if(r->services.broken)r->services.broken(r->services.context,id,&p);
   /*4FEF3C..4FEFAF: a player standing on this pier is thrown along the pier
    *axis in the whale's travel direction: 441280(0,sin*8,2,cos*8).*/
   if(r->services.standing&&r->services.throw_player&&r->services.sine&&r->services.standing(r->services.context,id)){
    uint16_t yaw=(uint16_t)p.angles[1];
    double c=(float)r->services.sine(r->services.context,(uint16_t)(yaw+16384)); /*7883C0 cos, stored*/
    double s=r->services.sine(r->services.context,yaw);                           /*788290 sin*/
    if(!isfinite(c)||!isfinite(s))return 0;
    if((double)f.whale_movement[1]*c+s*(double)f.whale_movement[0]<0){c=-c;s=-s;}
    float v[3]={(float)(s*8.),2.f,(float)(c*8.)};r->services.throw_player(r->services.context,v);r->throws++;
   }
  }
  return 1;
 }
 if(e->action==1){
  /*4FEDA6: timer++ then >300 selects action6 and40B7E0 deferred deletion.*/
  e->timer++;
  if(e->timer>r->assets->broken_ticks){e->action=6;return defer_free(l,id,e);}
  /*40B420: children run in creation order; a new child constructs first.*/
  for(uint32_t i=0;i<e->piece_count;i++){
   SA3DSPierPiece*q=&e->pieces[i];
   if(!q->constructed)construct_piece(r,e,q);else update_piece(q);
  }
  return 1;
 }
 return 1;
}
/*4FF060: variant from the row, DynamicCOL(68000001) at SET position/yawY,
 *then main/display/destructor installation. The4F88A0 stub returns0.*/
static int construct_pier(SA3DSOwnedTaskList*l,uint32_t id,void*context){
 (void)context;SA3DSOwnedTask*t=sa3ds_owned_task(l,id);SA3DSPierEntry*e=t?t->data:NULL;if(!e)return 0;
 SA3DSPierRuntime*r=e->owner;SA3DSPierPlacement p;if(!sa3ds_pier_placement(r->assets,e->row,&p))return 0;
 e->action=0;e->timer=0;
 if(!r->services.dynamic_add(r->services.context,id,&p))return 0;
 e->dynamic=1;e->displayed=1;t->destructor=destruct_pier;r->constructs++;
 return sa3ds_owned_task_set_main(l,id,main_pier);
}
int sa3ds_piers_init(SA3DSPierRuntime*r,const SA3DSPierAssets*a,SA3DSOwnedTaskList*l,SA3DSPierEntry*entries,
 const uint16_t*flags,uint32_t capacity,const SA3DSPierServices*s){
 if(!r||!a||!a->rows||!l||!l->slots||!entries||!flags||capacity<a->count||!s||!s->read_frame||!s->dynamic_add||!s->dynamic_remove)return 0;
 memset(r,0,sizeof(*r));r->assets=a;r->tasks=l;r->entries=entries;r->services=*s;
 memset(entries,0,(size_t)a->count*sizeof(*entries));
 for(uint32_t i=0;i<a->count;i++){entries[i].owner=r;entries[i].row=i;entries[i].lifetime.flags=flags[i];}
 return 1;
}
int sa3ds_piers_load(SA3DSPierRuntime*r){
 if(!r||!r->assets||!r->services.read_frame)return 0;
 SA3DSPierFrame f;if(!r->services.read_frame(r->services.context,&f)||f.clip_level>2)return 0;
 const SA3DSSetClipReference*reference=f.clip.player0.present?&f.clip.player0:&f.clip.camera;
 if(!reference->present)return 1;
 for(uint32_t i=0;i<r->assets->count;i++){
  SA3DSPierEntry*e=&r->entries[i];SA3DSPierPlacement p;
  if(!sa3ds_pier_placement(r->assets,i,&p))return 0;
  if(p.act!=f.act||!sa3ds_set_task_load_eligible(&e->lifetime))continue;
  if(e->lifetime.flags&2)return 0; /*restored SET payload not imported*/
  unsigned high=(p.type>>12)&7;
  if(high&&(high==1?f.clip_level>=1:f.clip_level>=2))continue;
  int inside;if(!sa3ds_set_position_in_range(p.position,reference->position,r->assets->load_distance_squared,&inside))return 0;
  if(!inside)continue;
  uint32_t id=sa3ds_owned_task_allocate(r->tasks,construct_pier,e);
  if(!id){r->allocation_failures++;continue;}
  if(!sa3ds_set_task_load_commit(&e->lifetime,id,r->assets->load_distance_squared))return 0;
 }
 return 1;
}
