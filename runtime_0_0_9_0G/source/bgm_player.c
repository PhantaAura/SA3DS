/*Native BGM: streams an owner ADX file from SD through ndsp. Decoding is the
 *portable src/game/adx.c; this file only owns file reads and wave buffers.
 *ndsp needs the DSP component (dspfirm.cdc on hardware); without it the
 *player stays off and the game runs silent.*/
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "../../src/game/adx.h"
#include "bgm_player.h"
#define BGM_FRAMES_PER_BUFFER 128u /*4096 samples per buffer*/
#define BGM_BUFFERS 3u
static struct {
 int ndsp_ready,playing;FILE*file;SA3DSAdxInfo info;SA3DSAdxState state;
 int16_t*pcm;ndspWaveBuf wave[BGM_BUFFERS];uint8_t header[0x800];uint32_t underruns,frames,loops;
} g;
int sa3ds_bgm_init(void){
 if(g.ndsp_ready)return 1;
 if(R_FAILED(ndspInit()))return 0;
 ndspSetOutputMode(NDSP_OUTPUT_STEREO);g.ndsp_ready=1;return 1;
}
static int fill(ndspWaveBuf*w){
 int16_t*out=(int16_t*)w->data_vaddr;uint32_t n=0;uint8_t frame[36];
 while(n<BGM_FRAMES_PER_BUFFER){
  if(g.info.loop&&g.state.sample>=g.info.loop_end){
   uint32_t at=sa3ds_adx_loop(&g.info,&g.state);if(!at||fseek(g.file,(long)at,SEEK_SET))return 0;g.loops++;
  }
  size_t bytes=(size_t)g.info.block_size*g.info.channels;
  if(fread(frame,1,bytes,g.file)!=bytes)return 0;
  int16_t tmp[64];if(!sa3ds_adx_decode_frame(&g.info,&g.state,frame,tmp))return 0;
  if(g.info.channels==2)memcpy(out+n*64,tmp,sizeof tmp);
  else for(unsigned i=0;i<32;i++){out[n*64+i*2]=tmp[i];out[n*64+i*2+1]=tmp[i];}
  n++;g.frames++;
 }
 w->nsamples=n*32;DSP_FlushDataCache(out,n*32*4);return 1;
}
int sa3ds_bgm_play(const char*path){
 sa3ds_bgm_stop();
 if(!g.ndsp_ready||!path)return 0;
 g.file=fopen(path,"rb");if(!g.file)return 0;
 size_t got=fread(g.header,1,sizeof g.header,g.file);
 if(!sa3ds_adx_open(&g.info,g.header,got)||fseek(g.file,(long)g.info.data_offset,SEEK_SET)){fclose(g.file);g.file=NULL;return 0;}
 memset(&g.state,0,sizeof g.state);
 if(!g.pcm)g.pcm=linearAlloc(BGM_BUFFERS*BGM_FRAMES_PER_BUFFER*32*4);
 if(!g.pcm){fclose(g.file);g.file=NULL;return 0;}
 ndspChnReset(0);ndspChnSetInterp(0,NDSP_INTERP_LINEAR);ndspChnSetRate(0,(float)g.info.rate);
 ndspChnSetFormat(0,NDSP_FORMAT_STEREO_PCM16);
 float mix[12]={0};mix[0]=mix[1]=0.8f;ndspChnSetMix(0,mix);
 memset(g.wave,0,sizeof g.wave);
 for(unsigned i=0;i<BGM_BUFFERS;i++){
  g.wave[i].data_vaddr=g.pcm+i*BGM_FRAMES_PER_BUFFER*32*2;
  if(!fill(&g.wave[i])){sa3ds_bgm_stop();return 0;}
  ndspChnWaveBufAdd(0,&g.wave[i]);
 }
 g.playing=1;return 1;
}
void sa3ds_bgm_update(void){
 if(!g.playing)return;
 for(unsigned i=0;i<BGM_BUFFERS;i++){
  if(g.wave[i].status!=NDSP_WBUF_DONE)continue;
  if(!fill(&g.wave[i])){sa3ds_bgm_stop();return;}
  ndspChnWaveBufAdd(0,&g.wave[i]);
 }
 if(!ndspChnIsPlaying(0))g.underruns++;
}
void sa3ds_bgm_stop(void){
 if(g.ndsp_ready)ndspChnReset(0);
 if(g.file){fclose(g.file);g.file=NULL;}
 g.playing=0;
}
void sa3ds_bgm_exit(void){sa3ds_bgm_stop();if(g.pcm){linearFree(g.pcm);g.pcm=NULL;}if(g.ndsp_ready){ndspExit();g.ndsp_ready=0;}}
void sa3ds_bgm_stats(SA3DSBgmStats*s){if(!s)return;s->ready=g.ndsp_ready;s->playing=g.playing;s->frames=g.frames;s->loops=g.loops;s->underruns=g.underruns;s->sample=g.state.sample;}
