#include "se_bank.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void w32(uint8_t*p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
int main(void){
 const SA3DSSeThreshold t[]={{1382,8},{861,4},{737,2},{65,1},{53,1},{39,1},{-1,-1}};uint8_t b;uint32_t i;
 assert(sa3ds_se_resolve(t,7,10,&b,&i)&&b==0&&i==10);
 assert(sa3ds_se_resolve(t,7,39,&b,&i)&&b==0&&i==39); /*strict >*/
 assert(sa3ds_se_resolve(t,7,40,&b,&i)&&b==1&&i==0);
 assert(sa3ds_se_resolve(t,7,0x2f,&b,&i)&&b==1&&i==7);
 assert(sa3ds_se_resolve(t,7,800,&b,&i)&&b==2&&i==62);
 assert(!sa3ds_se_resolve(t,7,0x569,&b,&i)); /*voice range*/
 uint8_t bank[256]={0};memcpy(bank,"archive  V2.DMZ",15);w32(bank+0x10,2);
 w32(bank+0x14,0x40);w32(bank+0x18,0x80);w32(bank+0x1c,4);w32(bank+0x20,0x50);w32(bank+0x24,0x90);w32(bank+0x28,4);
 strcpy((char*)bank+0x40,"B01_00_07.WAV");strcpy((char*)bank+0x50,"B01_00_08.WAV");
 SA3DSSoundBankEntry e;assert(sa3ds_se_find(bank,sizeof bank,8,&e)&&e.data==bank+0x90&&e.size==4);
 assert(sa3ds_se_find(bank,sizeof bank,7,&e)&&e.data==bank+0x80);assert(!sa3ds_se_find(bank,sizeof bank,9,&e));
 printf("SE id resolution: PASS bank0 low ids, strict thresholds, bank/index split, voice exclusion, case-insensitive name tail lookup\n");
}
