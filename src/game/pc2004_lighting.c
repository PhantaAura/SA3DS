#include "pc2004_lighting.h"
#include "sa3dspak.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
_Static_assert(sizeof(SA3DSLight8)==104,"D3DLIGHT8 snapshot layout");
_Static_assert(offsetof(SA3DSLight8,direction)==64,"D3DLIGHT8 direction offset");
_Static_assert(sizeof(SA3DSMaterial8)==68,"D3DMATERIAL8 snapshot layout");
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
int sa3ds_lighting_assets_open(SA3DSLightingAssets*out,const void*bytes,size_t size){
 if(!out)return 0;
 memset(out,0,sizeof(*out));SA3DSPak pak;SA3DSPakChunk c,m;
 if(!bytes||size>65536||sa3dspak_open(&pak,bytes,size)!=SA3DSPAK_OK||pak.toc_count!=2||
    sa3dspak_find(&pak,"METADATA",&m)!=SA3DSPAK_OK||m.flags||
    sa3dspak_find(&pak,"LIGHTING",&c)!=SA3DSPAK_OK||c.flags||c.size!=272)return 0;
 const uint8_t*b=c.data;
 if(memcmp(b,"SA3LITE\0",8)||r32(b+8)!=1||r32(b+12)!=2||r32(b+16)!=32||r32(b+20)!=80||r32(b+24)!=176||r32(b+28)!=272)return 0;
 for(unsigned i=0;i<3;i++){
  const uint8_t*p=b+(i?80+(i-1)*96:32);unsigned n=i?96:48;
  for(unsigned o=4;o<n;o+=4)if(!isfinite(rf(p+o)))return 0;
  if(i&&(p[0]!=1||p[1]!=0||p[2]!=0))return 0; /*exact ordinaryEC fallback profile*/
 }
 *out=(SA3DSLightingAssets){b+32,{b+80,b+176}};return 1;
}
int sa3ds_ec_character_lights(const SA3DSLightingAssets*a,unsigned act,SA3DSLight8 lights[4],uint8_t*mask){
 if(!a||!a->stage||act>1||!a->character[act]||!lights||!mask)return 0;
 memset(lights,0,4*sizeof(*lights));SA3DSLight8*p=&lights[0];const uint8_t*c=a->character[act];
 p->type=3;p->diffuse[3]=1;
 for(unsigned k=0;k<3;k++){
  p->diffuse[k]=rf(c+16);p->specular[k]=rf(c+52+k*4);p->ambient[k]=rf(c+20+k*4);
  p->direction[k]=rf((c[3]&2?c:a->stage)+4+k*4);
  if(c[3]&4)p->direction[k]=-p->direction[k];
 }
 *mask=1;
 if(c[3]&8){lights[3]=*p;for(unsigned k=0;k<3;k++)lights[3].direction[k]=-p->direction[k];*mask|=8;}
 return 1;
}
static void unpack(uint32_t word,float out[4]){out[0]=(float)((word>>16)&255);out[1]=(float)((word>>8)&255);out[2]=(float)(word&255);out[3]=(float)(word>>24);}
int sa3ds_pc2004_basic_material8(uint32_t diffuse,uint32_t specular,float power,SA3DSMaterial8*out){
 if(!out||!isfinite(power))return 0;
 memset(out,0,sizeof(*out));unpack(diffuse,out->diffuse);memcpy(out->ambient,out->diffuse,sizeof(out->ambient));unpack(specular,out->specular);out->power=power;return 1;
}

static float dot3(const float*a,const float*b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static int unit3(float*v){float q=dot3(v,v);if(!isfinite(q)||q<=0)return 0;float k=1/sqrtf(q);for(unsigned i=0;i<3;i++)v[i]*=k;return 1;}
static float saturate(float x){return fminf(1,fmaxf(0,x));}
int sa3ds_pc2004_light_vertex(const SA3DSLight8 lights[4],uint8_t mask,
    const SA3DSMaterial8*m,const float color[4],const float ambient[3],
    const float normal[3],const float position[3],const float eye[3],int normalize_normal,
    float diffuse[4],float specular[4]){
 if(!lights||!m||!color||!ambient||!normal||!position||!eye||!diffuse||!specular||
    !isfinite(color[3])||!isfinite(m->power)||m->power<0||(mask&~15u))return 0;
 float n[3],view[3];
 for(unsigned k=0;k<3;k++){
  if(!isfinite(normal[k])||!isfinite(position[k])||!isfinite(eye[k])||!isfinite(ambient[k])||
     !isfinite(color[k])||!isfinite(m->specular[k]))return 0;
  n[k]=normal[k];view[k]=eye[k]-position[k];
  diffuse[k]=color[k]*ambient[k];specular[k]=0;
 }
 if(normalize_normal)unit3(n);
 int valid_view=unit3(view);
 for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){
  const SA3DSLight8*l=&lights[i];if(l->type!=3)return 0;
  float toward[3],half[3];
  for(unsigned k=0;k<3;k++){
   if(!isfinite(l->direction[k])||!isfinite(l->diffuse[k])||!isfinite(l->ambient[k])||!isfinite(l->specular[k]))return 0;
   toward[k]=-l->direction[k];
  }
  int valid_light=unit3(toward);
  float facing=valid_light?fmaxf(0,dot3(n,toward)):0;
  float highlight=0;
  if(facing>0&&valid_view){
   for(unsigned k=0;k<3;k++)half[k]=toward[k]+view[k];
   if(unit3(half))highlight=powf(fmaxf(0,dot3(n,half)),m->power);
  }
  for(unsigned k=0;k<3;k++){
   diffuse[k]+=color[k]*(l->ambient[k]+l->diffuse[k]*facing);
   specular[k]+=m->specular[k]*l->specular[k]*highlight;
  }
 }
 for(unsigned k=0;k<3;k++){diffuse[k]=saturate(diffuse[k]);specular[k]=saturate(specular[k]);}
 diffuse[3]=saturate(color[3]);specular[3]=0;return 1;
}

int sa3ds_pc2004_normal_matrix(const float a[16],const float b[16],float out[9]){
 if(!a||!b||!out)return 0;
 float m[9];for(unsigned col=0;col<3;col++)for(unsigned row=0;row<3;row++)
  m[col*3+row]=a[row]*b[col*4]+a[4+row]*b[col*4+1]+a[8+row]*b[col*4+2];
 float cof[9]={m[4]*m[8]-m[5]*m[7],m[5]*m[6]-m[3]*m[8],m[3]*m[7]-m[4]*m[6],
  m[7]*m[2]-m[8]*m[1],m[8]*m[0]-m[6]*m[2],m[6]*m[1]-m[7]*m[0],
  m[1]*m[5]-m[2]*m[4],m[2]*m[3]-m[0]*m[5],m[0]*m[4]-m[1]*m[3]};
 float det=m[0]*cof[0]+m[1]*cof[1]+m[2]*cof[2];if(!isfinite(det)||det==0)return 0;
 for(unsigned i=0;i<9;i++)out[i]=cof[i]/det;
 return 1;
}
