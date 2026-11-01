#include "ring_runtime.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static void *read_file(const char *path,size_t *n){
 FILE*f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long len=ftell(f);assert(len>0);rewind(f);
 void*p=malloc((size_t)len);assert(p&&fread(p,1,(size_t)len,f)==(size_t)len);fclose(f);*n=(size_t)len;return p;
}
static unsigned tick(SA3DSRings*r,float x,int debug,int eligible,unsigned clip){
 return sa3ds_rings_tick(r,(SA1Vec3){x,0,0},(SA1Vec3){x,0,0},4,debug,eligible,clip);
}
int main(int argc,char**argv){
 assert(argc>=2);size_t n;void*p=read_file(argv[1],&n);SA3DSRings r;
 if(argc==3){assert(!sa3ds_rings_open(&r,p,n));assert(!r.states);free(p);return 0;}
 assert(sa3ds_rings_open(&r,p,n)&&r.count==4&&r.clip_level==1);
 assert(!tick(&r,0,0,1,1)&&r.states[0].action==4);
 assert(!tick(&r,0,0,1,1)&&r.states[0].action==5);
 assert(!tick(&r,0,0,1,1)&&r.states[0].action==1&&r.states[0].yaw==100);
 assert(!r.states[1].action&&r.states[2].action==1); /* source clip-level branches */
 assert(!tick(&r,9,0,1,1)&&!r.states[0].pending_hit); /* strict radius boundary */
 assert(!tick(&r,8.99f,0,1,1)&&r.states[0].pending_hit&&r.rings==0);
 assert(tick(&r,8.99f,0,1,1)==1&&r.rings==1&&r.collected_events==1);
 for(int i=0;i<10;i++)assert(!tick(&r,0,0,1,1));assert(r.rings==1);
 sa3ds_rings_reset_act(&r,0);assert(!r.states[0].collected&&r.states[0].yaw==100);
 for(int i=0;i<4;i++)assert(!tick(&r,0,1,1,0)); /* debug suppresses queue/turn */
 assert(r.states[0].action==1&&r.states[0].yaw==100&&!r.states[0].pending_hit);
 assert(!tick(&r,0,0,0,0)&&r.states[0].pending_hit);
 assert(!tick(&r,0,0,0,0)&&r.states[0].collected&&r.rings==1); /* status4000 consumes without tally */
 sa3ds_rings_reset_act(&r,0);for(int i=0;i<4;i++)tick(&r,20,0,1,0);
 assert(r.states[0].action==1);
 r.states[0].yaw=65530;tick(&r,20,0,1,0);assert(r.states[0].yaw==358);
 tick(&r,600.05f,0,1,0);assert(r.states[0].action==1); /* unload range is loadSq+100, not radius+100 */
 tick(&r,601,0,1,0);assert(!r.states[0].action);
 tick(&r,20,0,1,0);assert(r.states[0].action==4&&r.states[0].yaw==100);
 unsigned old=r.states[0].action;
 assert(!sa3ds_rings_tick(&r,(SA1Vec3){NAN,0,0},(SA1Vec3){0,0,0},4,0,1,0));assert(r.states[0].action==old);
 r.rings=99;assert(sa3ds_rings_add(&r,1)==1&&r.rings==100);
 r.rings=999;assert(sa3ds_rings_add(&r,1)==1&&r.rings==1000); /* no999 cap */
 r.rings=32767;assert(!sa3ds_rings_add(&r,1)&&r.rings==INT16_MIN);
 sa3ds_rings_reset_act(&r,1);for(int i=0;i<5;i++)tick(&r,0,0,1,0);assert(r.states[3].collected);
 sa3ds_rings_destroy(&r);assert(!r.states&&!r.rows);free(p);
 puts("Direct ring runtime: PASS (strict sphere, queued collision, source init/turn/clipping, one tally, unload/respawn, signed counter)");
}
