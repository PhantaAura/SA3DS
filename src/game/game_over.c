#include "game_over.h"
#include <string.h>
int sa3ds_game_over_tick(SA3DSGameOver*m,uint32_t pressed,SA3DSGameOverResult*out){
    if(!m||!out)return 0;
    memset(out,0,sizeof(*out));if(m->finished)return 1;
    if(!m->initialized){m->initialized=1;m->selection=0;m->timer=1200;out->initialize=1;out->music=0x1a;}
    /*Confirm precedes timeout, and both precede selection toggle.*/
    if(pressed&0xcu){
        out->cleanup=out->release_textures=1;
        out->retry=m->selection==0;out->reset_lives=out->retry;out->quit=!out->retry;m->finished=1;return 1;
    }
    if(m->timer<1){m->timer=(int32_t)((uint32_t)m->timer-1u);out->cleanup=out->release_textures=out->quit=1;m->finished=1;return 1;}
    if(pressed&0x300c0u)m->selection^=1;
    m->timer=(int32_t)((uint32_t)m->timer-1u);out->draw=1;return 1;
}
int sa3ds_game_over_sprites(SA3DSGameOver*m,SA3DSGameOverSprite out[9]){
    if(!m||!out||m->selection>1)return 0;
    int phase=m->pulse<128?m->pulse:(int)m->pulse-256;
    if(phase<0)phase=-phase;
    float f=phase*.0078125f;m->pulse=(uint8_t)((m->pulse+4u)&252u);
    unsigned b=(unsigned)(f*255.),r=(unsigned)(255.-f*255.);
    uint32_t selected=0xff000000u|(r<<16)|b,blue=0xff0000ffu;
    uint32_t glow=0xff000000u|((unsigned)(f*19.+236.)<<16)|((unsigned)(f*83.+152.)<<8);
    SA3DSGameOverSprite calls[9]={
        {0,0,0,10,80,60,0xffffffffu}, /*433C90 explicit640x480 rectangle from8x8*/
        {1,192,160,1.04f,1,1,0xffffffffu},
        {4,160,352,2,1,1,m->selection==0?selected:blue},
        {4,352,352,2,1,1,m->selection==1?selected:blue},
        {3,160,352,1.1f,1,1,0xffffffffu},
        {3,352,352,1.1f,1,1,0xffffffffu},
        {2,m->selection?352:160,352,1.05f,1,1,glow},
        {5,160,368,1,.5f,.5f,0xffffffffu},
        {6,352,368,1,.5f,.5f,0xffffffffu}};
    memcpy(out,calls,sizeof(calls));return 1;
}
