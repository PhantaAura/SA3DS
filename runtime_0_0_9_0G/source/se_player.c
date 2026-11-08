/*Native sound effects: owner SE banks (unchanged .dat files staged by
 *tools/stage_pc2004_se.py) resolved by src/game/se_bank.c and played on ndsp
 *channels 1..8. Clips are decoded whole on play (mono PCM16, <=12 s each).*/
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/game/se_bank.h"
#include "../../src/sa3dspak/sa3dspak.h"
#include "se_player.h"
#define SE_VOICES 8u
#define SE_MAX_SAMPLES (22050u*12u) /*longest EC/Sonic clip is 11.3 s*/
static struct {
 int ready;uint8_t*bank[8];size_t bank_size[8];SA3DSSeThreshold thresh[80];size_t thresh_count;
 int16_t*pcm;ndspWaveBuf wave[SE_VOICES];unsigned next;uint32_t played,missing,truncated;
} g;
static uint8_t*read_all(const char*path,size_t*size){
 FILE*f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);
 uint8_t*b=n>0?malloc((size_t)n):NULL;if(b&&fread(b,1,(size_t)n,f)!=(size_t)n){free(b);b=NULL;}fclose(f);if(b)*size=(size_t)n;return b;
}
int sa3ds_se_init(const char*dir){
 if(g.ready)return 1;
 char path[160];snprintf(path,sizeof path,"%s/se_map.sa3dspak",dir);size_t n=0;uint8_t*map=read_all(path,&n);if(!map)return 0;
 SA3DSPak pak;SA3DSPakChunk t,s;int ok=sa3dspak_open(&pak,map,n)==SA3DSPAK_OK&&sa3dspak_find(&pak,"SE_THRESH",&t)==SA3DSPAK_OK&&
  sa3dspak_find(&pak,"SE_SLOTS",&s)==SA3DSPAK_OK&&t.size%8==0&&t.size/8<=80&&s.size%32==0;
 if(ok){
  g.thresh_count=(size_t)(t.size/8);memcpy(g.thresh,t.data,(size_t)t.size); /*little-endian int32 pairs*/
  for(size_t i=0;i<s.size/32;i++){
   uint8_t slot=s.data[i*32];char name[32];memcpy(name,s.data+i*32+1,31);name[31]=0;
   if(slot>=8)continue;
   snprintf(path,sizeof path,"%s/%s.dat",dir,name);
   g.bank[slot]=read_all(path,&g.bank_size[slot]);if(!g.bank[slot]||!sa3ds_sound_bank_count(g.bank[slot],g.bank_size[slot]))ok=0;
  }
 }
 free(map);
 if(ok&&!g.pcm)g.pcm=linearAlloc(SE_VOICES*SE_MAX_SAMPLES*2);
 if(!ok||!g.pcm){sa3ds_se_exit();return 0;}
 for(unsigned v=0;v<SE_VOICES;v++){ndspChnReset(1+v);ndspChnSetInterp(1+v,NDSP_INTERP_LINEAR);ndspChnSetFormat(1+v,NDSP_FORMAT_MONO_PCM16);}
 g.ready=1;return 1;
}
int sa3ds_se_play(uint32_t id){
 if(!g.ready)return 0;
 uint8_t slot;uint32_t index;SA3DSSoundBankEntry e;SA3DSAdxInfo in;
 if(!sa3ds_se_resolve(g.thresh,g.thresh_count,id,&slot,&index)||slot>=8||!g.bank[slot]||
    !sa3ds_se_find(g.bank[slot],g.bank_size[slot],index,&e)||!sa3ds_adx_open(&in,e.data,e.size)){g.missing++;return 0;}
 unsigned v=g.next++%SE_VOICES;int16_t*out=g.pcm+v*SE_MAX_SAMPLES;
 ndspChnWaveBufClear(1+v);
 SA3DSAdxState st;memset(&st,0,sizeof st);size_t pos=in.data_offset,fb=(size_t)in.block_size*in.channels;uint32_t n=0;int16_t tmp[64];
 while(st.sample<in.total_samples&&pos+fb<=e.size&&n+32<=SE_MAX_SAMPLES){
  if(!sa3ds_adx_decode_frame(&in,&st,e.data+pos,tmp))break;
  pos+=fb;
  for(unsigned i=0;i<32;i++)out[n+i]=in.channels==2?(int16_t)((tmp[i*2]+tmp[i*2+1])/2):tmp[i];
  n+=32;
 }
 if(st.sample<in.total_samples)g.truncated++;
 if(n>in.total_samples)n=in.total_samples;
 if(!n)return 0;
 DSP_FlushDataCache(out,n*2);
 ndspChnSetRate(1+v,(float)in.rate);float mix[12]={0};mix[0]=mix[1]=0.9f;ndspChnSetMix(1+v,mix);
 memset(&g.wave[v],0,sizeof g.wave[v]);g.wave[v].data_vaddr=out;g.wave[v].nsamples=n;
 ndspChnWaveBufAdd(1+v,&g.wave[v]);g.played++;return 1;
}
void sa3ds_se_exit(void){
 for(unsigned v=0;v<SE_VOICES&&g.ready;v++)ndspChnReset(1+v);
 for(unsigned i=0;i<8;i++){free(g.bank[i]);g.bank[i]=NULL;}
 if(g.pcm){linearFree(g.pcm);g.pcm=NULL;}g.ready=0;
}
void sa3ds_se_stats(uint32_t*played,uint32_t*missing,uint32_t*truncated){if(played)*played=g.played;if(missing)*missing=g.missing;if(truncated)*truncated=g.truncated;}
