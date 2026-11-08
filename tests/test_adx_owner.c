#include "adx.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char**argv){
 for(int a=1;a<argc;a++){
  if(strstr(argv[a],".dat")){continue;}
  FILE*f=fopen(argv[a],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);
  uint8_t*b=malloc((size_t)n);assert(b&&fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);
  SA3DSAdxInfo in;assert(sa3ds_adx_open(&in,b,(size_t)n));
  assert(in.loop&&in.loop_start_byte==in.data_offset+(in.loop_start/32u)*in.block_size*in.channels);
  SA3DSAdxState st;memset(&st,0,sizeof st);int16_t out[64];unsigned long clips=0,frames=0;size_t pos=in.data_offset;
  while(st.sample<in.loop_end&&pos+(size_t)in.block_size*in.channels<=(size_t)n){
   assert(sa3ds_adx_decode_frame(&in,&st,b+pos,out));pos+=(size_t)in.block_size*in.channels;frames++;
   for(unsigned i=0;i<32u*in.channels;i++)if(out[i]==32767||out[i]==-32768)clips++;
  }
  assert(st.loop_saved&&st.sample>=in.loop_end);
  uint32_t back=sa3ds_adx_loop(&in,&st);assert(back==in.loop_start_byte&&st.sample==in.loop_start);
  const char*name=strrchr(argv[a],'/');
  printf("ADX owner %s: PASS %u ch %u Hz, %u samples, loop %u..%u (byte check ok), %lu frames decoded, %lu clipped samples\n",
   name?name+1:argv[a],in.channels,in.rate,in.total_samples,in.loop_start,in.loop_end,frames,clips);
  free(b);
 }
 for(int a=1;a<argc;a++){
  if(!strstr(argv[a],".dat"))continue;
  FILE*f=fopen(argv[a],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);
  uint8_t*b=malloc((size_t)n);assert(b&&fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);
  uint32_t c=sa3ds_sound_bank_count(b,(size_t)n);assert(c>0);unsigned long samples=0;unsigned mono=0,rates=0;
  for(uint32_t i=0;i<c;i++){
   SA3DSSoundBankEntry e;assert(sa3ds_sound_bank_entry(b,(size_t)n,i,&e));
   SA3DSAdxInfo in;assert(sa3ds_adx_open(&in,e.data,e.size));mono+=in.channels==1;rates+=in.rate==22050;
   SA3DSAdxState st;memset(&st,0,sizeof st);int16_t out[64];size_t pos=in.data_offset,fb=(size_t)in.block_size*in.channels;
   while(st.sample<in.total_samples&&pos+fb<=e.size){assert(sa3ds_adx_decode_frame(&in,&st,e.data+pos,out));pos+=fb;}
   assert(st.sample>=in.total_samples);samples+=in.total_samples;
  }
  const char*name=strrchr(argv[a],'/');
  printf("SE bank owner %s: PASS %u ADX entries (%u mono, %u at 22050 Hz), %lu samples decoded\n",name?name+1:argv[a],c,mono,rates,samples);
  free(b);
 }
}
