#include "set_object_clip.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
 float p[3]={3,4,0},origin[3]={0};int inside;
 assert(sa3ds_set_position_in_range(p,origin,25,&inside)&&inside);
 p[2]=.001f;assert(sa3ds_set_position_in_range(p,origin,25,&inside)&&!inside);
 p[0]=6;assert(sa3ds_set_position_in_range(p,origin,25,&inside)&&!inside);
 float unload;assert(sa3ds_set_unload_distance(160000,&unload)&&unload==160100);
 assert(sa3ds_set_unload_distance(-100,&unload)&&unload==0);
 SA3DSSetClipInput i={.has_set_data=1,.unload_distance_squared=160100};uint8_t remove;
 assert(sa3ds_set_object_clip(&i,&remove)&&remove); /*No camera/player references.*/
 i.camera.present=1;i.camera.position[0]=400;assert(sa3ds_set_object_clip(&i,&remove)&&!remove);
 i.camera.position[0]=401;assert(sa3ds_set_object_clip(&i,&remove)&&remove);
 i.player0.present=1;i.player0.position[0]=400;assert(sa3ds_set_object_clip(&i,&remove)&&!remove);
 i.player0.present=0;i.player1.present=1;i.player1.position[0]=40;
 assert(sa3ds_set_object_clip(&i,&remove)&&!remove); /*Sonic player1 gets fixed1600.*/
 i.player1.position[0]=nextafterf(40,INFINITY);assert(sa3ds_set_object_clip(&i,&remove)&&remove);
 i.player1.position[0]=0;i.player1_character=1;assert(sa3ds_set_object_clip(&i,&remove)&&remove);
 i.player1.present=0;i.camera.present=0;i.set_flags=8;
 assert(sa3ds_set_object_clip(&i,&remove)&&!remove);
 i.set_flags=0;i.unload_distance_squared=0;assert(sa3ds_set_object_clip(&i,&remove)&&!remove);
 i.has_set_data=0;i.camera.present=1;i.camera.position[0]=410;
 assert(sa3ds_set_object_clip(&i,&remove)&&!remove); /*Default168100 inclusive.*/
 i.camera.position[0]=nextafterf(410,INFINITY);assert(sa3ds_set_object_clip(&i,&remove)&&remove);
 assert(!sa3ds_set_position_in_range(NULL,origin,25,&inside));
 puts("SET object clip: PASS inclusive range,load+100 store,camera/player0 retention,character-gated player1 fixed40-unit range,no-reference deletion,flag8/zero bypass and default410-unit boundary; native task replacement separate");
}
