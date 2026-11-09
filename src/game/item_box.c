#include "item_box.h"
#include <string.h>
int sa3ds_item_box_kind(float s){
 int k=(int)s;          /*4D6F6x ftol*/
 if(k<0)return 8;if(k>8)return 0;return k;
}
static const int16_t RANDOM_RINGS[6]={1,5,10,15,20,40};
int sa3ds_item_box_award(int kind,uint8_t character,uint16_t(*rand15)(void*),void*ctx,SA3DSItemAward*o){
 if(!o||kind<0||kind>8)return 0;
 memset(o,0,sizeof *o);o->icon=(uint8_t)kind;
 switch(kind){
 case SA3DS_ITEM_RINGS5:o->rings=5;break;
 case SA3DS_ITEM_RINGS10:o->rings=10;break;
 case SA3DS_ITEM_RINGS_RANDOM:{
  if(!rand15)return 0;
  int i=(int)((float)((float)rand15(ctx)*3.0517578125e-05f)*-6.0f); /*0..-5*/
  o->rings=RANDOM_RINGS[-i];break;}
 case SA3DS_ITEM_LIFE:o->lives=1;break;
 default:o->unported=1;return 1;
 }
 if(character==6)o->sounds[o->sound_count++]=0x51a;
 o->sounds[o->sound_count++]=0xb;
 return 1;
}
