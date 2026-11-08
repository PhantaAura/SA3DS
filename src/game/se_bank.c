#include "se_bank.h"
#include <stdio.h>
#include <ctype.h>
int sa3ds_se_resolve(const SA3DSSeThreshold*t,size_t n,uint32_t id,uint8_t*bank,uint32_t*index){
 if(!t||!bank||!index||id>0x568)return 0;
 for(size_t i=0;i<n&&t[i].threshold>=0;i++){
  if((int32_t)id>t[i].threshold){*bank=(uint8_t)t[i].bank;*index=id-(uint32_t)t[i].threshold-1u;return 1;}
 }
 *bank=0;*index=id;return 1;
}
static int same_tail(const char*a,const char*b){ /*lstrcmpiA from the 4th char*/
 for(size_t i=0;i<3;i++)if(!a[i]||!b[i])return 0;
 a+=3;b+=3;while(*a&&*b){if(tolower((unsigned char)*a)!=tolower((unsigned char)*b))return 0;a++;b++;}
 return *a==*b;
}
int sa3ds_se_find(const uint8_t*bank,size_t size,uint32_t index,SA3DSSoundBankEntry*out){
 if(!bank||!out||index>99)return 0;
 char want[24];snprintf(want,sizeof want,"B%02u_00_%02u.wav",0u,(unsigned)index);
 uint32_t c=sa3ds_sound_bank_count(bank,size);
 for(uint32_t i=0;i<c;i++){SA3DSSoundBankEntry e;if(sa3ds_sound_bank_entry(bank,size,i,&e)&&same_tail(e.name,want)){*out=e;return 1;}}
 return 0;
}
