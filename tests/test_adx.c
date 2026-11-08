#include "adx.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void w16(uint8_t*p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
int main(void){
 uint8_t h[0x40]={0};h[0]=0x80;w16(h+2,0x40-4);h[4]=3;h[5]=18;h[6]=4;h[7]=2;w32(h+8,44100);w32(h+12,96);w16(h+16,500);h[18]=3;
 w32(h+0x18,1);w32(h+0x1c,32);w32(h+0x20,0x40+36);w32(h+0x24,96);memcpy(h+0x40-6,"(c)CRI",6);
 SA3DSAdxInfo in;assert(sa3ds_adx_open(&in,h,sizeof h)&&in.channels==2&&in.data_offset==0x40&&in.loop&&in.loop_start==32&&in.loop_end==96);
 assert(in.coef1==7334&&in.coef2==-3284); /*floor rounding*/
 uint8_t bad[0x40];memcpy(bad,h,sizeof h);bad[4]=4;assert(!sa3ds_adx_open(&in,bad,sizeof bad));
 memcpy(bad,h,sizeof h);bad[0x3a]='x';assert(!sa3ds_adx_open(&in,bad,sizeof bad));
 assert(sa3ds_adx_open(&in,h,sizeof h));
 /*Frame: ch0 scale 9 (->10) nibbles +1,-1,7,-8...; ch1 silent.*/
 uint8_t f[36]={0};w16(f,9);f[2]=0x1f;f[3]=0x78;
 SA3DSAdxState st;memset(&st,0,sizeof st);int16_t out[64];
 assert(sa3ds_adx_decode_frame(&in,&st,f,out));
 int32_t s0=10,s1=-10+((7334*s0)>>12),s2=70+((7334*s1-3284*s0)>>12),s3=-80+((7334*s2-3284*s1)>>12);
 assert(out[0]==s0&&out[2]==s1&&out[4]==s2&&out[6]==s3&&out[1]==0&&st.sample==32&&!st.loop_saved);
 /*Second frame starts at loop_start: history saved before decoding.*/
 int32_t h1=st.h1[0],h2=st.h2[0];assert(sa3ds_adx_decode_frame(&in,&st,f,out)&&st.loop_saved&&st.loop_h1[0]==h1&&st.loop_h2[0]==h2);
 assert(sa3ds_adx_decode_frame(&in,&st,f,out)&&st.sample==96);
 assert(sa3ds_adx_loop(&in,&st)==0x40+36&&st.sample==32&&st.h1[0]==h1&&st.h2[0]==h2);
 /*Clamp.*/
 uint8_t loud[36]={0};w16(loud,0x7fff);memset(loud+2,0x77,16);SA3DSAdxState z;memset(&z,0,sizeof z);
 assert(sa3ds_adx_decode_frame(&in,&z,loud,out)&&out[0]==32767);
 printf("ADX decoder: PASS v3 header/loop fields, floor coefficients, encoding/copyright rejection, nibble order+scale+1+prediction, loop history save/restore, clamp\n");
}
