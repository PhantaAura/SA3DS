#include "player_position_history.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 SA3DSPlayerPositionHistory h={0};float out[3]={7,8,9},p[3]={1,2,3};
 assert(!sa3ds_player_position_history_read(&h,0,out)&&out[0]==7);
 assert(!sa3ds_player_position_history_record(&h,p)&&h.cursor==0);
 sa3ds_player_position_history_present(&h,1);
 assert(sa3ds_player_position_history_read(&h,255,out)&&out[0]==0&&out[1]==0&&out[2]==0);
 for(unsigned frame=1;frame<=513;frame++){
  p[0]=(float)frame;p[1]=-(float)frame;p[2]=(float)frame*.25f;
  assert(sa3ds_player_position_history_record(&h,p)&&h.cursor==(uint8_t)frame);
  for(unsigned offset=0;offset<256;offset++){
   assert(sa3ds_player_position_history_read(&h,(uint8_t)offset,out));
   if(offset>=frame)assert(out[0]==0&&out[1]==0&&out[2]==0);
   else assert(out[0]==frame-offset&&out[1]==-(float)(frame-offset)&&out[2]==(frame-offset)*.25f);
  }
 }
 uint8_t cursor=h.cursor;uint32_t bits[3]={0x80000000,0x7fc12345,0x7f800000};memcpy(p,bits,12);
 assert(sa3ds_player_position_history_write(&h,255,p)&&h.cursor==cursor);
 assert(sa3ds_player_position_history_read(&h,255,out)&&!memcmp(out,bits,12));
 assert(sa3ds_player_position_history_read(&h,0,NULL)&&sa3ds_player_position_history_write(&h,0,NULL));
 sa3ds_player_position_history_present(&h,0);assert(!sa3ds_player_position_history_write(&h,0,p));
 sa3ds_player_position_history_present(&h,1);assert(h.cursor==cursor&&sa3ds_player_position_history_read(&h,255,out)&&!memcmp(out,bits,12));
 puts("Player position history: PASS513 writes/all256 offsets,cursor wrap,cold zero slots,presence persistence,no-age-clamp,nonadvancing setter and exact float-word copies; native scheduling separate");
}
