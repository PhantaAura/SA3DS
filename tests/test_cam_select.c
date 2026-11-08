#include "cam_select.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static double sine(void*c,uint16_t a){(void)c;if(!(a&0x3fff)){const double v[]={0,1,0,-1};return v[a>>14];}return sin(a*6.2831853071795864769/65536.0);}
static uint8_t prio[256],apply[256],aflag[256];
static SA3DSCamRecord sphere(uint8_t mode,uint8_t sub,uint8_t adj,float x,float r){SA3DSCamRecord v={mode,sub,adj,0,0,0,{x,0,0},{r,0,r*r},{0,0,0},{0,0,0}};return v;}
static SA3DSCamRecord box(uint8_t mode,uint8_t sub,uint8_t adj,float x,float h){SA3DSCamRecord v={mode,sub,adj,2,0,0,{x,0,0},{h,h,h},{0,0,0},{0,0,0}};return v;}
static int step(SA3DSCamSelect*s,SA3DSCamTables*t,SA3DSCamRecord*r,uint32_t n,float x){
 float p[3]={x,-4,0};return sa3ds_cam_select_update(s,t,r,n,0,p,p,p,sine,0);}
int main(void){
 prio[4]=0;prio[31]=0;prio[21]=1;prio[40]=1;prio[22]=3;prio[50]=2;prio[51]=2;prio[61]=4;
 for(unsigned i=0;i<256;i++)apply[i]=(uint8_t)(i%3+1);aflag[18]=1;
 SA3DSCamTables t={prio,apply,aflag};SA3DSCamSelect s;
 sa3ds_cam_select_init(&s,&t,4,0);assert(s.priority==0&&s.record==-1&&s.mode==4&&s.adjust==0&&s.apply==apply[4]);
 /*Record0 is never visited, even when the player stands in it.*/
 SA3DSCamRecord r[6]={sphere(31,0,7,0,10),sphere(31,0,18,0,10),box(21,0,19,100,10),box(40,5,20,100,10),sphere(50,0,1,300,10),sphere(51,3,2,300,10)};
 assert(step(&s,&t,r,1,0)&&s.record==-1&&s.mode==4);
 /*Priority0 enter: same level, saved[0] keeps mode4 with adjust 0x15 for record -1.*/
 assert(step(&s,&t,r,2,0)&&s.record==1&&s.mode==31&&s.priority==0&&s.adjust==18&&(s.flags&1));
 assert(s.saved[0].mode==4&&s.saved[0].record==-1&&s.saved[0].adjust==0x15);
 /*Priority0 never leaves by itself.*/
 assert(step(&s,&t,r,2,50)&&s.record==1&&s.mode==31);
 /*Priority1 box entered (records count-1..1: record3 has higher sub but is visited first).*/
 s.changed=0;assert(step(&s,&t,r,3,100)&&s.priority==1&&s.record==2&&s.mode==21&&s.changed);
 assert(s.saved[1].priority==0&&s.saved[1].record==1&&s.saved[1].mode==31&&s.saved[1].adjust==18);
 /*Same-level higher sub-priority replaces: leave (back to 31) then enter 40.*/
 assert(step(&s,&t,r,4,100)&&s.priority==1&&s.record==3&&s.mode==40);
 assert(s.saved[1].record==1&&s.saved[1].mode==31);
 /*Leaving the box returns to the saved prio0 state and clears frames.*/
 s.reset_frames=0;assert(step(&s,&t,r,4,200)&&s.priority==0&&s.record==1&&s.mode==31&&s.reset_frames&&s.apply==apply[31]);
 /*Priority2: enter 50, then a higher sub-priority prio2 record pops (436140) and enters.*/
 assert(step(&s,&t,r,5,300)&&s.priority==2&&s.record==4&&s.mode==50);
 assert(step(&s,&t,r,6,300)&&s.priority==2&&s.record==5&&s.mode==51);
 assert(s.saved[2].priority==0&&s.saved[2].mode==31);
 /*Prio4 gate at level2 retargets the return state's adjust (mode61 keeps mode).*/
 SA3DSCamRecord g[7];memcpy(g,r,sizeof r);g[6]=(SA3DSCamRecord){61,0,23,1,0,0,{300,0,0},{30,30,0},{0,0,0},{0,0,0}};
 float p1[3]={300,-4,1},p2[3]={300,-4,-1};
 assert(sa3ds_cam_select_update(&s,&t,g,7,0,p1,p1,p2,sine,0)&&s.saved[2].adjust==23&&s.saved[2].mode==31&&s.mode==51);
 /*Default mode restore after a prio2 pop that returns to level0.*/
 SA3DSCamSelect d;sa3ds_cam_select_init(&d,&t,4,0);d.default_mode=4;d.default_adjust=0;
 assert(step(&d,&t,r,5,300)&&d.priority==2);
 assert(step(&d,&t,r,6,300)&&d.mode==51&&d.saved[2].mode==4&&d.saved[2].record==-1);
 /*Adjust zeroed when both sub-priorities are >=10 (435DE0).*/
 SA3DSCamRecord hi[3]={sphere(31,0,0,0,1),box(21,12,19,0,10),box(40,13,20,0,10)};SA3DSCamSelect h;sa3ds_cam_select_init(&h,&t,4,0);
 float z[3]={0,-4,0};
 assert(sa3ds_cam_select_update(&h,&t,hi,2,12,z,z,z,sine,0)&&h.record==1&&h.adjust==0); /*header sub12 vs rec12*/
 assert(sa3ds_cam_select_update(&h,&t,hi,3,12,z,z,z,sine,0)&&h.record==2&&h.adjust==0);
 SA3DSCamSelect l;sa3ds_cam_select_init(&l,&t,4,0);
 assert(sa3ds_cam_select_update(&l,&t,hi,2,0,z,z,z,sine,0)&&l.record==1&&l.adjust==19); /*header sub0 <10*/
 /*Idle/event modes hold.*/
 h.mode=76;assert(step(&h,&t,hi,3,1000)&&h.record==2);
 printf("CAM selector 437FB0: PASS record0 skip, prio0 enter/hold, prio1 enter/replace/leave, prio2 enter/pop, prio4 gate retarget, default restore, sub>=10 adjust zero, idle hold\n");
}
