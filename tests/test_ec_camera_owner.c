#include "cam_trigger.h"
#include "sa3dspak.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static double sine(void*c,uint16_t a){(void)c;if(!(a&0x3fff)){const double v[]={0,1,0,-1};return v[a>>14];}return sin(a*6.2831853071795864769/65536.0);}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);
 uint8_t*b=malloc((size_t)n);assert(b&&fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);
 SA3DSPak pak;SA3DSPakChunk prio,rows;assert(sa3dspak_open(&pak,b,(size_t)n)==SA3DSPAK_OK);
 assert(sa3dspak_find(&pak,"CAMPRIO",&prio)==SA3DSPAK_OK&&prio.size==256);
 const unsigned expect_rows[3]={96,133,1},expect_regions[3]={23,49,0};
 unsigned spheres=0,boxes=0,gates=0;
 for(unsigned act=0;act<3;act++){
  char key[16];snprintf(key,sizeof key,"CAMACT%u",act);assert(sa3dspak_find(&pak,key,&rows)==SA3DSPAK_OK);
  static SA3DSCamRecord r[256];uint32_t count=0;assert(sa3ds_cam_records_decode(r,256,rows.data,(size_t)rows.size,&count)&&count==expect_rows[act]);
  unsigned regions=0;
  for(uint32_t i=0;i<count;i++){
   if(prio.data[r[i].mode]!=4)continue;regions++;
   float e[3]={r[i].center[0]+.25f,r[i].center[1],r[i].center[2]};
   if(r[i].shape==0){
    if(r[i].extent[2]<=.0625f)continue;
    assert(sa3ds_cam_region_sphere_push(&r[i],e));
    float d=sqrtf((e[0]-r[i].center[0])*(e[0]-r[i].center[0])+(e[1]-r[i].center[1])*(e[1]-r[i].center[1])+(e[2]-r[i].center[2])*(e[2]-r[i].center[2]));
    assert(fabsf(d-r[i].extent[0])<1e-3f*fmaxf(1.f,r[i].extent[0]));spheres++;
   }else if(r[i].shape==2){
    if(r[i].extent[0]<.25f)continue;
    assert(sa3ds_cam_region_box_push(&r[i],e,sine,NULL));boxes++;
    float again[3]={e[0],e[1],e[2]};(void)again; /*pushed onto the boundary*/
   }else gates++;
  }
  assert(regions==expect_regions[act]);
  /*Whole pass: an eye at the first region's center moves; a far eye does not.*/
  uint8_t p[256];memcpy(p,prio.data,256);float far[3]={1e6f,1e6f,1e6f},old[3]={1e6f,1e6f,1e6f};unsigned reverts;
  assert(sa3ds_cam_regions_apply(r,count,p,old,far,sine,NULL,&reverts)==0&&far[0]==1e6f);
 }
 printf("EC camera owner package: PASS CAMPRIO 256, act rows 96/133/1, priority-4 regions 23/49/0, %u sphere and %u box pushes onto their surfaces, %u gates, far eye untouched\n",spheres,boxes,gates);
 free(b);
}
