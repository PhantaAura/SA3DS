#include "course_clock.h"
static int signed_byte(uint8_t n){return n&128?(int)n-256:n;}
void sa3ds_course_clock_reset(SA3DSCourseClock*c){*c=(SA3DSCourseClock){.enabled=1};}
void sa3ds_course_clock_tick(SA3DSCourseClock*c){
    c->updates++;
    if(!c->enabled)return;
    c->time[2]++;
    if(signed_byte(c->time[2])>59){
        c->time[2]=0;c->time[1]++;
        if(signed_byte(c->time[1])>59){
            c->time[1]=0;c->time[0]++;
            if(signed_byte(c->time[0])>99){c->time[0]=99;c->time[1]=c->time[2]=59;}
        }
    }
}
