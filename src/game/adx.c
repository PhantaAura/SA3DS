#include "adx.h"
#include <math.h>
#include <string.h>
static uint32_t be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t be16(const uint8_t*p){return (uint16_t)(p[0]<<8|p[1]);}
int sa3ds_adx_open(SA3DSAdxInfo*o,const uint8_t*h,size_t n){
 if(!o||!h||n<0x18||h[0]!=0x80||h[1]!=0)return 0;
 memset(o,0,sizeof *o);
 o->data_offset=(uint32_t)be16(h+2)+4u;
 if(o->data_offset>n||o->data_offset<0x1a||memcmp(h+o->data_offset-6,"(c)CRI",6))return 0;
 if(h[4]!=3||h[5]!=18||h[6]!=4||h[7]<1||h[7]>2)return 0;    /*encoding3 only*/
 if(h[0x13]&0x08)return 0;                                  /*encrypted flags unsupported*/
 o->block_size=h[5];o->channels=h[7];o->rate=be32(h+8);o->total_samples=be32(h+12);
 uint16_t hp=be16(h+16);uint8_t ver=h[18];
 if(!o->rate||!hp)return 0;
 if(ver==3&&o->data_offset>=0x2c+6){o->loop=(uint8_t)(be32(h+0x18)!=0);o->loop_start=be32(h+0x1c);o->loop_start_byte=be32(h+0x20);o->loop_end=be32(h+0x24);}
 else if(ver==4&&o->data_offset>=0x38+6){o->loop=(uint8_t)(be32(h+0x24)!=0);o->loop_start=be32(h+0x28);o->loop_start_byte=be32(h+0x2c);o->loop_end=be32(h+0x30);}
 if(o->loop&&(o->loop_start>=o->loop_end||o->loop_end>o->total_samples||o->loop_start%32))o->loop=0;
 const double sq2=1.4142135623730950488,pi=3.1415926535897932385;
 double a=sq2-cos(2.0*pi*hp/o->rate),b=sq2-1.0,c=(a-sqrt((a+b)*(a-b)))/b;
 o->coef1=(int32_t)floor(c*8192.0);o->coef2=(int32_t)floor(c*c*-4096.0);
 return 1;
}
int sa3ds_adx_decode_frame(const SA3DSAdxInfo*o,SA3DSAdxState*s,const uint8_t*f,int16_t*out){
 if(!o||!s||!f||!out)return 0;
 if(o->loop&&!s->loop_saved&&s->sample==o->loop_start){
  memcpy(s->loop_h1,s->h1,sizeof s->h1);memcpy(s->loop_h2,s->h2,sizeof s->h2);s->loop_saved=1;
 }
 for(unsigned ch=0;ch<o->channels;ch++){
  const uint8_t*b=f+ch*o->block_size;int32_t scale=(int32_t)be16(b)+1,h1=s->h1[ch],h2=s->h2[ch];
  for(unsigned i=0;i<32;i++){
   int32_t nib=(i&1)?(b[2+i/2]&15):(b[2+i/2]>>4);if(nib>=8)nib-=16;
   int32_t v=nib*scale+((o->coef1*h1+o->coef2*h2)>>12);
   if(v>32767)v=32767;else if(v<-32768)v=-32768;
   h2=h1;h1=v;out[i*o->channels+ch]=(int16_t)v;
  }
  s->h1[ch]=h1;s->h2[ch]=h2;
 }
 s->sample+=32;return 1;
}
uint32_t sa3ds_adx_loop(const SA3DSAdxInfo*o,SA3DSAdxState*s){
 if(!o||!s||!o->loop||!s->loop_saved)return 0;
 memcpy(s->h1,s->loop_h1,sizeof s->h1);memcpy(s->h2,s->loop_h2,sizeof s->h2);s->sample=o->loop_start;
 return o->data_offset+(o->loop_start/32u)*o->block_size*o->channels;
}
static uint32_t le32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
uint32_t sa3ds_sound_bank_count(const uint8_t*b,size_t n){
 if(!b||n<0x14||memcmp(b,"archive  V2.DMZ",15))return 0;
 uint32_t c=le32(b+0x10);if(c>4096||0x14+(size_t)c*12>n)return 0;return c;
}
int sa3ds_sound_bank_entry(const uint8_t*b,size_t n,uint32_t i,SA3DSSoundBankEntry*o){
 uint32_t c=sa3ds_sound_bank_count(b,n);if(!o||i>=c)return 0;
 const uint8_t*e=b+0x14+(size_t)i*12;uint32_t no=le32(e),off=le32(e+4),sz=le32(e+8);
 if(no>=n||off>n||sz>n-off)return 0;
 size_t len=0;while(no+len<n&&b[no+len])len++;if(no+len>=n)return 0;
 o->name=(const char*)b+no;o->data=b+off;o->size=sz;return 1;
}
