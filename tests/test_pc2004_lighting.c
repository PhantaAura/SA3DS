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
 for(unsigned k=0;k<3;k++)assert(lights[0].diffuse[k]==1.002f&&lights[0].specular[k]==.805f&&lights[0].ambient[k]==.75f);
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
 /*Hand-computed directional/Gouraud cases, separate from the owner table.*/
 SA3DSLight8 single[4]={0};single[0].type=3;single[0].direction[1]=-2;
 for(unsigned k=0;k<3;k++){single[0].ambient[k]=.1f;single[0].diffuse[k]=.4f;single[0].specular[k]=.8f;m.specular[k]=.5f;}
 m.power=2;float color[4]={1,.5f,.25f,.75f},ambient[3]={0,0,0};
 float normal[3]={0,1,0},position[3]={0,0,0},eye[3]={0,2,0},diffuse[4],specular[4];
 assert(sa3ds_pc2004_light_vertex(single,1,&m,color,ambient,normal,position,eye,0,diffuse,specular));
 assert(fabsf(diffuse[0]-.5f)<1e-6f&&fabsf(diffuse[1]-.25f)<1e-6f&&fabsf(diffuse[2]-.125f)<1e-6f&&diffuse[3]==.75f);
 assert(fabsf(specular[0]-.4f)<1e-6f&&specular[3]==0);
 eye[0]=2;eye[1]=0;
 assert(sa3ds_pc2004_light_vertex(single,1,&m,color,ambient,normal,position,eye,0,diffuse,specular));
 assert(fabsf(specular[0]-.2f)<1e-6f); /*Halfway vector, not reflection-vector Phong.*/
 normal[1]=-1;assert(sa3ds_pc2004_light_vertex(single,1,&m,color,ambient,normal,position,eye,0,diffuse,specular));
 assert(fabsf(diffuse[0]-.1f)<1e-6f&&specular[0]==0);
 normal[1]=2;eye[0]=0;eye[1]=2;
 assert(sa3ds_pc2004_light_vertex(single,1,&m,color,ambient,normal,position,eye,0,diffuse,specular)&&fabsf(diffuse[0]-.9f)<1e-6f);
 assert(sa3ds_pc2004_light_vertex(single,1,&m,color,ambient,normal,position,eye,1,diffuse,specular)&&fabsf(diffuse[0]-.5f)<1e-6f);
 m.specular[0]=255;assert(sa3ds_pc2004_light_vertex(single,1,&m,color,ambient,normal,position,eye,1,diffuse,specular)&&specular[0]==1);
 m.power=-1;assert(!sa3ds_pc2004_light_vertex(single,1,&m,color,ambient,normal,position,eye,1,diffuse,specular));
 float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 float transformed[16]={0,2,0,0,-3,0,0,0,0,0,4,0,17,23,29,1},nm[9];
 assert(sa3ds_pc2004_normal_matrix(identity,transformed,nm));
 assert(fabsf(nm[1]-.5f)<1e-6f&&fabsf(nm[3]+1.f/3)<1e-6f&&fabsf(nm[8]-.25f)<1e-6f&&nm[0]==0);
 float scale[16]={2,0,0,0,0,3,0,0,0,0,4,0,0,0,0,1};
 float rotate[16]={0,1,0,0,-1,0,0,0,0,0,1,0,0,0,0,1};
 assert(sa3ds_pc2004_normal_matrix(rotate,scale,nm));
 assert(fabsf(nm[1]-.5f)<1e-6f&&fabsf(nm[3]+1.f/3)<1e-6f);
 transformed[10]=0;assert(!sa3ds_pc2004_normal_matrix(identity,transformed,nm));
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
 puts("Lighting state: PASS source API layout,own/stage/reverse/back lights,raw material channels,act bounds,directional diffuse/specular and inverse-transpose transforms; native raster separate");
}
