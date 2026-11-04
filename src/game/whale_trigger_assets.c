#include "whale_trigger_assets.h"
#include "sa3dspak.h"
#include <math.h>
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
int sa3ds_whale_trigger_placement(const SA3DSWhaleTriggerAssets*a,uint32_t i,SA3DSWhalePlacement*out){
 if(!a||!a->rows||!out||i>=a->count)return 0;
 const uint8_t*p=a->rows+i*40;SA3DSWhalePlacement row={.act=r32(p),.source_row=r32(p+4),.type=r16(p+8)};
 for(unsigned k=0;k<3;k++){
  uint16_t word=r16(p+10+k*2);memcpy(&row.angles[k],&word,2);
  row.position[k]=rf(p+16+k*4);row.parameters[k]=rf(p+28+k*4);
 }
 *out=row;return 1;
}
int sa3ds_whale_trigger_assets_open(SA3DSWhaleTriggerAssets*out,const void*bytes,size_t size){
 if(!out)return 0;
 memset(out,0,sizeof(*out));SA3DSPak pak;SA3DSPakChunk c,m;
 if(!bytes||size>65536||sa3dspak_open(&pak,bytes,size)!=SA3DSPAK_OK||pak.toc_count!=2||
    sa3dspak_find(&pak,"METADATA",&m)!=SA3DSPAK_OK||m.flags||
    sa3dspak_find(&pak,"WHALE_TRIG",&c)!=SA3DSPAK_OK||c.flags||c.size<48)return 0;
 const uint8_t*b=c.data;uint32_t n=r32(b+12),spec=r32(b+20);
 if(memcmp(b,"SA3WTRIG",8)||r16(b+8)!=1||r16(b+10)||n>64||r32(b+16)!=32||
    spec!=32+n*40||r32(b+24)!=16||r32(b+28)!=spec+16||c.size!=spec+16||
    rf(b+spec)!=45||rf(b+spec+4)!=160000||r32(b+spec+8)||r32(b+spec+12))return 0;
 SA3DSWhaleTriggerAssets next={b+32,n,rf(b+spec),rf(b+spec+4)};SA3DSWhalePlacement previous={0};
 for(uint32_t i=0;i<n;i++){
  SA3DSWhalePlacement row;if(!sa3ds_whale_trigger_placement(&next,i,&row))return 0;
  if(row.act>1||row.source_row>=65536||((row.type&4095)!=67&&(row.type&4095)!=68)||
     (i&&(row.act<previous.act||(row.act==previous.act&&row.source_row<=previous.source_row))))return 0;
  for(unsigned k=0;k<3;k++)if(!isfinite(row.position[k])||!isfinite(row.parameters[k]))return 0;
  previous=row;
 }
 *out=next;return 1;
}
int sa3ds_whale_trigger_inside(const float c[3],float radius,const SA3DSWhaleContactPlayer players[2],uint8_t*out){
 if(!c||!players||!out||!isfinite(radius)||radius<0)return 0;
 for(unsigned k=0;k<3;k++)if(!isfinite(c[k]))return 0;
 *out=0;
 for(unsigned player=0;player<2;player++){
  if(!players[player].task_present||!players[player].ccl_present)continue;
  float d[3];for(unsigned k=0;k<3;k++){
   if(!isfinite(players[player].center[k]))return 0;
   d[k]=(float)((double)players[player].center[k]-c[k]);
  }
  /*787CC0 adds Z²+Y², then X², stores the sum to float, then7871F0
   *square-roots that stored value. Retain strict distance-radius<0.*/
  double squared=(double)d[2]*d[2]+(double)d[1]*d[1];squared+=(double)d[0]*d[0];
  if(sqrt((double)(float)squared)-(double)radius<0){*out=(uint8_t)(player+1);break;}
 }
 return 1;
}
