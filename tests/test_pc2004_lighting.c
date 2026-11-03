#include "pc2004_lighting.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void wf(unsigned char*p,float f){memcpy(p,&f,4);}
int main(int argc,char**argv){
 unsigned char stage[48]={0},profile[96]={1,0,0,3};
 wf(stage+4,1);wf(stage+8,2);wf(stage+12,3);
 wf(profile+4,0);wf(profile+8,-1);wf(profile+12,-.033f);
 wf(profile+16,1.002f);
 for(unsigned k=0;k<3;k++){wf(profile+20+k*4,.75f);wf(profile+52+k*4,.805f);}
 SA3DSLightingAssets a={stage,{profile,profile}};SA3DSLight8 lights[4];uint8_t mask;
 assert(sa3ds_ec_character_lights(&a,0,lights,&mask)&&mask==1);
 assert(lights[0].type==3&&lights[0].diffuse[3]==1&&lights[0].specular[3]==0&&lights[0].ambient[3]==0);
 assert(lights[0].direction[1]==-1&&lights[0].direction[2]==-.033f);
 for(unsigned k=0;k<3;k++)assert(lights[0].diffuse[k]==1.002f&&lights[0].specular[k]==.75f&&lights[0].ambient[k]==.805f);
 assert(lights[0].range==0&&lights[0].falloff==0&&!lights[1].type&&!lights[2].type&&!lights[3].type);
 profile[3]=12;assert(sa3ds_ec_character_lights(&a,1,lights,&mask)&&mask==9);
 for(unsigned k=0;k<3;k++)assert(lights[0].direction[k]==-(float)(k+1)&&lights[3].direction[k]==(float)(k+1));
 assert(!sa3ds_ec_character_lights(&a,2,lights,&mask));
 SA3DSMaterial8 m;
 assert(sa3ds_pc2004_basic_material8(0x12345678,0xff0000b1,11,&m));
 assert(m.diffuse[0]==52&&m.diffuse[1]==86&&m.diffuse[2]==120&&m.diffuse[3]==18);

 for(unsigned v=0;v<256;v++){
  uint32_t color=(v<<24)|(v<<16)|(v<<8)|v;
  assert(sa3ds_pc2004_basic_material8(color,0xff0000b1u,11,&m));
  for(unsigned k=0;k<4;k++)assert(m.diffuse[k]==v&&m.ambient[k]==v&&m.emissive[k]==0);
  assert(m.specular[0]==0&&m.specular[1]==0&&m.specular[2]==177&&m.specular[3]==255&&m.power==11);
 }
 if(argc>1){
  FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);void*b=malloc(n);assert(b&&fread(b,1,n,f)==(size_t)n);fclose(f);
  assert(sa3ds_lighting_assets_open(&a,b,n));
  for(unsigned act=0;act<2;act++){
   assert(sa3ds_ec_character_lights(&a,act,lights,&mask)&&mask==1);
   printf("%u",act);float values[25];memcpy(values,(const char*)&lights[0]+4,sizeof(values));const float*p=values;
   printf(" %u",lights[0].type);for(unsigned k=0;k<25;k++)printf(" %.9g",p[k]);putchar('\n');
  }
  ((unsigned char*)b)[n-1]^=1;assert(!sa3ds_lighting_assets_open(&a,b,n)&&!a.stage);free(b);
  for(int i=2;i<argc;i++){
   f=fopen(argv[i],"rb");assert(f);fseek(f,0,SEEK_END);n=ftell(f);rewind(f);b=malloc(n);assert(b&&fread(b,1,n,f)==(size_t)n);fclose(f);
   assert(!sa3ds_lighting_assets_open(&a,b,n)&&!a.stage);free(b);
  }
 }
 puts("Lighting state: PASS source API layout,own/stage/reverse/back lights,all byte material channels without normalization and act bounds; native shading separate");
}
