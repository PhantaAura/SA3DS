#include "game_over.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 SA3DSGameOver m={0};SA3DSGameOverResult r;
 for(int i=0;i<1200;i++){assert(sa3ds_game_over_tick(&m,0,&r));assert(r.draw&&!r.quit&&!r.cleanup&&m.timer==1199-i);assert(r.initialize==(i==0));if(!i)assert(r.music==0x1a);}
 assert(sa3ds_game_over_tick(&m,0,&r)&&r.quit&&r.cleanup&&r.release_textures&&m.timer==-1);
 assert(sa3ds_game_over_tick(&m,4,&r)&&!r.retry&&!r.quit&&!r.draw);
 m=(SA3DSGameOver){0};assert(sa3ds_game_over_tick(&m,0x300c0,&r)&&m.selection==1&&m.timer==1199);
 assert(sa3ds_game_over_tick(&m,8,&r)&&r.quit&&!r.retry);
 m=(SA3DSGameOver){.initialized=1,.timer=0};assert(sa3ds_game_over_tick(&m,0x300c4,&r)&&r.retry&&r.reset_lives&&!r.quit&&m.selection==0);
 for(unsigned phase=0;phase<256;phase++)for(unsigned selection=0;selection<2;selection++){
  SA3DSGameOverSprite s[9];m=(SA3DSGameOver){.pulse=phase,.selection=selection};
  assert(sa3ds_game_over_sprites(&m,s)&&m.pulse==((phase+4)&252));
  assert(s[0].texture==0&&s[0].scale_x==80&&s[0].scale_y==60&&s[0].z==10);
  assert(s[1].x==192&&s[1].y==160&&s[1].texture==1);
  assert(s[6].texture==2&&s[6].x==(selection?352:160));
  assert(s[7].texture==5&&s[8].texture==6&&s[7].scale_x==.5f&&s[8].y==368);
  unsigned distance=phase<128?phase:256-phase;
  unsigned blue=distance*255/128,red=255-blue-(distance*255%128!=0);
  assert(s[2+selection].argb==(0xff000000u|(red<<16)|blue));
  assert(s[3-selection].argb==0xff0000ffu);
  assert(s[6].argb==(0xff000000u|((236+distance*19/128)<<16)|((152+distance*83/128)<<8)));
 }
 puts("Game-over menu: PASS1200 draws/1201st timeout,confirm priority,toggle,cleanup and all pulse/selection sprite commands; native screen separate");
}
