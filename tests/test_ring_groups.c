#include "ring_groups.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static void *read_file(const char*path,size_t*n){
 FILE*f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long len=ftell(f);rewind(f);assert(len>0);
 void*p=malloc((size_t)len);assert(p&&fread(p,1,(size_t)len,f)==(size_t)len);fclose(f);*n=(size_t)len;return p;
}
static unsigned tick(SA3DSRings*r,SA1Vec3 p,int presented){
 return sa3ds_rings_tick_presented(r,p,p,4,0,1,r->clip_level,presented);
}
int main(int argc,char**argv){
 assert(argc>=2);size_t size;void*bytes=read_file(argv[1],&size);SA3DSRings r;
 if(argc==3){assert(!sa3ds_rings_open(&r,bytes,size));assert(!r.states&&!r.groups&&!r.actors);free(bytes);return 0;}
 assert(sa3ds_rings_open(&r,bytes,size));assert(r.direct_count==4&&r.count==13&&r.group_count==2);
 assert(sa3ds_ring_group_sin(&r,0)==0&&sa3ds_ring_group_sin(&r,16384)==1);
 assert(sa3ds_ring_group_sin(&r,32768)==0&&sa3ds_ring_group_sin(&r,49152)==-1);
 assert(sa3ds_ring_group_sin(&r,1)==1.0/16384&&sa3ds_ring_group_sin(&r,65535)==-1.0/16384);
 SA3DSRingGroup*g=&r.groups[0];assert(g->first_child==4&&g->child_count==5);
 const float z[5]={0,10,-10,20,-20};
 for(unsigned i=0;i<5;i++){
  SA3DSRingPlacement p;assert(sa3ds_rings_placement(&r,4+i,&p));
  assert(p.group==0&&p.child_index==i&&p.position.x==300&&p.position.y==0&&p.position.z==z[i]);
  assert(!p.angles[0]&&!p.angles[1]&&!p.angles[2]&&!p.parameters[0]&&!p.parameters[1]&&!p.parameters[2]);
 }
 /* Fractional count remains denominator; all-axis90 Z-X-Y transforms
  * the first circle child to parent+(-localX,localZ,0). */
 SA3DSRingPlacement circle;assert(sa3ds_rings_placement(&r,9,&circle));
 assert(circle.child_index==1&&fabsf(circle.position.x-980.050048828125f)<.0001f);
 assert(fabsf(circle.position.y-100.050048828125f)<.0001f&&circle.position.z==2000);
 SA1Vec3 p={300,0,0};assert(!tick(&r,p,1));assert(g->action==1&&r.states[4].action==0);
 for(int i=0;i<4;i++)assert(!tick(&r,p,1));
 assert(r.states[4].pending_hit&&r.states[4].action==1);
 assert(tick(&r,p,1)==1&&r.states[4].action==2&&r.states[4].pickup_age==-4&&!g->journal);
 assert(sa3ds_rings_draw_normal(&r,4)&&!sa3ds_rings_draw_normal(&r,4));
 assert(r.states[4].pickup_age==-4&&!g->journal); /* drawing cannot advance pickup/journal timing */
 for(int i=0;i<14;i++){tick(&r,p,1);assert(!g->journal);}
 assert(r.states[4].action==2);tick(&r,p,1);
 assert(r.states[4].action==3&&r.states[4].pickup_age==8&&g->journal==1);
 tick(&r,p,1);assert(r.states[4].consumed);
 /* A child crossing410 while its parent remains inside600 starts the
  * source parent3->4->delete sequence; its collection journal survives. */
 p.x=720;tick(&r,p,1);assert(g->action==3);
 tick(&r,p,1);assert(g->action==4);tick(&r,p,1);assert(!g->action&&g->journal==1);
 p.x=300;tick(&r,p,1);assert(g->action==1&&r.states[4].consumed&&r.states[4].collected);
 for(int i=0;i<8;i++)tick(&r,p,1);assert(r.rings==1);
 sa3ds_rings_reset_act(&r,1);assert(!r.groups[1].journal);
 p=circle.position;for(int i=0;i<5;i++)assert(!tick(&r,p,1));
 assert(tick(&r,p,1)==1&&r.states[9].action==2);
 for(int i=0;i<18;i++)tick(&r,p,0);
 assert(r.states[9].action==2&&!r.groups[1].journal); /* catch-up render guard */
 tick(&r,p,1);assert(r.states[9].action==3&&r.groups[1].journal==2); /* circlebit0 unused */
 sa3ds_rings_reset_act(&r,1);assert(!r.groups[1].journal&&!r.states[9].collected);
 sa3ds_rings_destroy(&r);free(bytes);
 puts("Ring groups: PASS (LUT quadrants/interpolation, source line/circle+ZXY, delayed children, pickup phase/journal, unload/reload/reset)");
}
