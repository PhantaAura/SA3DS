#include "set_preparation.h"
#include <string.h>
int sa3ds_set_cold_prepare(SA3DSSetTaskLifetime*e,const uint16_t*types,uint32_t count,
 uint8_t level,uint8_t act,int32_t mission){
 if(!e||(!types&&count)||count>0x3ff)return 0;
 for(uint32_t i=0;i<count;i++){
  int32_t composite=(int32_t)(types[i]&0xfff)+((int32_t)act*0x100+level)*0x100;
  if(composite==mission)return 0; /*46BD86 mission-table flags not imported.*/
 }
 memset(e,0,(size_t)count*sizeof(*e));
 for(uint32_t i=0;i<count;i++)e[i].flags=0x8000;
 return 1;
}
int sa3ds_set_act_clear(SA3DSSetTaskLifetime*e,uint32_t count,int(*request_free)(void*,uint32_t),void*context){
 if(!e||!request_free)return 0;
 for(uint32_t i=0;i<count;i++){
  uint16_t old=e[i].flags;if(old&0x4000)continue;
  e[i].flags=(uint16_t)(old&0x7fe9);
  if(old&1){
   /*46BE36 writes main=40B570 through entry+4; a null association would fault.*/
   if(!e[i].task||!request_free(context,e[i].task))return 0;
   e[i].free_main=1;
  }
 }
 return 1;
}
int sa3ds_set_act_compact(SA3DSSetTaskLifetime*e,uint32_t count){
 if(!e)return 0;
 for(uint32_t i=0;i<count;i++){
  if(e[i].flags&0x4000){e[i].flags&=(uint16_t)~0x4000u;continue;}
  if(e[i].flags&2)return 0;
  memset(&e[i],0,sizeof(e[i]));
 }
 return 1;
}
int sa3ds_set_restart_reset(SA3DSSetTaskLifetime*e,uint32_t count){
 if(!e)return 0;
 for(uint32_t i=0;i<count;i++)if(!(e[i].flags&0x10)){e[i].flags=0x8000;e[i].loads=0;}
 return 1;
}
