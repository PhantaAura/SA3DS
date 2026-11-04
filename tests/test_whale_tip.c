#include "whale_tip.h"
#include "whale_draw.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void wi(unsigned char*p,int32_t v){uint32_t u;memcpy(&u,&v,4);for(unsigned k=0;k<4;k++)p[k]=(unsigned char)(u>>(k*8));}
static double sine(void*c,uint16_t a){(void)c;switch(a){case 0:return 0;case 0x4000:return 1;case 0x8000:return 0;case 0xc000:return -1;default:return sin((double)a*6.2831853071795864769/65536);}}
int main(int argc,char**argv){
 unsigned char keys[128]={0};SA3DSWhaleTipAssets a={.pitch=keys};SA3DSWhaleMotionServices s={NULL,sine};
 SA3DSWhaleMotion m={0};m.action=4;m.position[0]=10;m.position[1]=20;m.position[2]=30;m.draw_y=7;
 float tip[3];int32_t pitch;
 assert(sa3ds_whale_tip(&m,&a,&s,tip,&pitch)&&pitch==0&&tip[0]==10&&tip[1]==22&&tip[2]==150);
 m.yaw=0x4000;assert(sa3ds_whale_tip(&m,&a,&s,tip,&pitch)&&tip[0]==130&&tip[1]==22&&tip[2]==30);
 wi(keys,0x4000);m.yaw=0;assert(sa3ds_whale_tip(&m,&a,&s,tip,&pitch)&&tip[0]==10&&tip[1]==-93&&tip[2]==25);
 wi(keys,-5);wi(keys+4,2);m.frame=.5f;
 assert(sa3ds_whale_tip(&m,&a,&s,tip,&pitch)&&pitch==-1); /*Truncation, not floor.*/
 wi(keys+29*4,100);wi(keys,300);wi(keys+30*4,1000);wi(keys+31*4,2000);
 m.frame=29.5f;assert(sa3ds_whale_tip(&m,&a,&s,tip,&pitch)&&pitch==200);
 m.frame=30;assert(sa3ds_whale_tip(&m,&a,&s,tip,&pitch)&&pitch==1000);
 m.action=1;assert(sa3ds_whale_tip(&m,&a,&s,tip,&pitch)&&tip[0]==0&&tip[1]==-100000&&tip[2]==0);
 m.frame=nextafterf(30,INFINITY);assert(!sa3ds_whale_tip(&m,&a,&s,tip,&pitch));
 m.frame=-.1f;assert(!sa3ds_whale_tip(&m,&a,&s,tip,&pitch));m.frame=NAN;assert(!sa3ds_whale_tip(&m,&a,&s,tip,&pitch));
 assert(!sa3ds_whale_root_pitch(&a,2,4,&pitch));a.exit_root_bind=1;
 assert(sa3ds_whale_root_pitch(&a,2,4,&pitch)&&pitch==0);
 assert(sa3ds_whale_root_pitch(&a,4,29.5f,&pitch)&&pitch==550); /*Cycle root, not tip's29->0.*/
 assert(!sa3ds_whale_root_pitch(&a,2,NAN,&pitch));
 for(int arg=1;arg<argc;arg++){
  FILE*f=fopen(argv[arg],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);void*b=malloc(n);assert(b&&fread(b,1,n,f)==(size_t)n);fclose(f);
  if(arg==1){assert(sa3ds_whale_tip_assets_open(&a,b,n));
   assert(a.exit_root_bind&&sa3ds_whale_root_pitch(&a,2,29.5f,&pitch)&&pitch==0);
   for(unsigned k=0;k<32;k++){uint32_t u=(uint32_t)a.pitch[k*4]|((uint32_t)a.pitch[k*4+1]<<8)|((uint32_t)a.pitch[k*4+2]<<16)|((uint32_t)a.pitch[k*4+3]<<24);int32_t value;memcpy(&value,&u,4);printf("KEY %u %d\n",k,value);}
   for(unsigned tick=0;tick<=256;tick++){int32_t root;assert(sa3ds_whale_cycle_pitch(&a,tick*.125f,&root));printf("ROOT %u %d\n",tick,root);}
   ((unsigned char*)b)[n-1]^=1;assert(!sa3ds_whale_tip_assets_open(&a,b,n)&&!a.pitch);
  }else assert(!sa3ds_whale_tip_assets_open(&a,b,n)&&!a.pitch);
  free(b);
 }
 puts("Whale tip: PASS Y/X rotation,pre-movement translation,negative truncation,29->0 blend,exact30 sentinel,bounds and emergence; native wiring/x87 parity separate");
}
