#include "item_box.h"
#include <assert.h>
#include <stdio.h>
static uint16_t value;static uint16_t r(void*c){(void)c;return value;}
int main(void){
 assert(sa3ds_item_box_kind(3.0f)==3&&sa3ds_item_box_kind(4.1f)==4&&sa3ds_item_box_kind(6.4f)==6&&sa3ds_item_box_kind(2.15f)==2);
 assert(sa3ds_item_box_kind(-0.5f)==0&&sa3ds_item_box_kind(-1.f)==8&&sa3ds_item_box_kind(9.f)==0&&sa3ds_item_box_kind(8.9f)==8);
 SA3DSItemAward a;
 assert(sa3ds_item_box_award(2,0,r,0,&a)&&a.rings==5&&a.sound_count==1&&a.sounds[0]==0xb);
 assert(sa3ds_item_box_award(3,0,r,0,&a)&&a.rings==10);
 assert(sa3ds_item_box_award(6,0,r,0,&a)&&a.lives==1&&a.rings==0);
 assert(sa3ds_item_box_award(2,6,r,0,&a)&&a.sound_count==2&&a.sounds[0]==0x51a&&a.sounds[1]==0xb);
 const struct{uint16_t v;int rings;}cases[]={{0,1},{5461,1},{5462,5},{16383,10},{16384,15},{21845,15},{21846,20},{27306,20},{27307,40},{32767,40}};
 for(unsigned i=0;i<sizeof cases/sizeof*cases;i++){value=cases[i].v;assert(sa3ds_item_box_award(4,0,r,0,&a)&&a.rings==cases[i].rings);}
 assert(sa3ds_item_box_award(1,0,r,0,&a)&&a.unported&&!a.sound_count);
 assert(!sa3ds_item_box_award(4,0,0,0,&a)&&!sa3ds_item_box_award(9,0,r,0,&a));
 printf("Item box rules: PASS kind trunc/clamps, 5/10 rings, 1-up, random table boundaries (1,5,10,15,20,40), sound 0xB (+0x51A for character 6), unported kinds flagged\n");
}
