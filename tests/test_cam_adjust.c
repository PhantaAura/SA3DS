#include "cam_adjust.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static double sine(void*c,uint16_t a){(void)c;return sin(a*6.2831853071795864769/65536.0);}
static int hit;static int ray(void*c,float s[3],float d[3],float r){(void)c;(void)s;(void)r;if(!hit)return 0;for(int k=0;k<3;k++)d[k]*=.5f;return 1;}
static int near(double a,double b){return fabs(a-b)<1e-3;}
static SA3DSCamera at(float x,float y,float z,int32_t rx,int32_t ry){SA3DSCamera c;memset(&c,0,sizeof c);c.position[0]=x;c.position[1]=y;c.position[2]=z;c.rot_x=rx;c.rot_y=ry;return c;}
int main(void){
 SA3DSCamServices sv={0,sine,ray,NULL};float p[3]={0,0,0};SA3DSCamAdjust a;memset(&a,0,sizeof a);uint8_t idx=0;
 SA3DSCamera o=at(0,10,40,-2000,0),n=at(40,10,0,-2000,16384);
 /*Adjust0 no-op; unported 1-5.*/
 SA3DSCamera keep=n;assert(sa3ds_cam_adjust_run(&n,&o,&a,&idx,4,p,p,0,&sv)&&!memcmp(&keep,&n,sizeof n));
 idx=3;assert(!sa3ds_cam_adjust_run(&n,&o,&a,&idx,4,p,p,0,&sv));
 /*468650 (idx18, 180 frames): t=0 keeps the old offset from the previous player position.*/
 idx=18;float prev[3]={-5,0,0};assert(sa3ds_cam_adjust_run(&n,&o,&a,&idx,4,p,prev,0,&sv));
 assert(near(n.position[0],5)&&near(n.position[2],40)&&n.rot_y==0&&a.counter==1&&a.duration==180&&a.use_ray==0);
 /*Midway t=90/180: halfway between offsets, rotation eased by |d|*t.*/
 a.counter=90;n=at(40,10,0,-2000,16384);assert(sa3ds_cam_adjust_run(&n,&o,&a,&idx,4,p,p,0,&sv));
 assert(near(n.position[0],20)&&near(n.position[2],20)&&n.rot_y==0x71c&&idx==18); /*eased 8192, clamped to lim_y=(0xE38-0)*.5*/
 /*Clamp 468790: rot_y kept within lim_y of the old camera's bearing (ay=0 here).*/
 assert(a.start_y==0); /*old camera already faced the player*/
 /*Finish: t>=1 -> adjust0, counter cleared, new kept (mode 0x4C -> 0xB).*/
 a.counter=180;n=at(40,10,0,-2000,16384);assert(sa3ds_cam_adjust_run(&n,&o,&a,&idx,4,p,p,0,&sv)&&idx==0&&a.counter==0&&near(n.position[0],40));
 idx=18;a.counter=180;assert(sa3ds_cam_adjust_run(&n,&o,&a,&idx,0x4c,p,p,0,&sv)&&idx==0xb);
 /*4684E0 (idx10, frames split 10): counter<frames scales by counter/frames.*/
 idx=10;memset(&a,0,sizeof a);a.counter=0;n=at(40,10,0,-2000,0);assert(sa3ds_cam_adjust_run(&n,&o,&a,&idx,4,p,p,0,&sv));
 assert(near(n.position[0],0)&&near(n.position[2],40)); /*t=0: v=0*/
 a.counter=5;n=at(40,10,0,-2000,0);assert(sa3ds_cam_adjust_run(&n,&o,&a,&idx,4,p,p,0,&sv));
 {float t=5.f/600.f,s=1-t;double vx=((40-0)+(0-0)*s*.2)*t,vz=((0-40)+(40*s*.2f))*t;assert(near(n.position[0],5*vx/10)&&near(n.position[2],40+5*vz/10));}
 /*Odd entries run 462CF0: hit keeps old+delta/2; turning keeps 10%.*/
 idx=19;memset(&a,0,sizeof a);hit=1;n=at(40,10,0,0,0);
 assert(sa3ds_cam_adjust_run(&n,&o,&a,&idx,4,p,p,0,&sv)&&a.use_ray==1);  /*t=0: n already = o, ray moves nothing*/
 a.counter=90;n=at(40,10,0,0,0);assert(sa3ds_cam_adjust_run(&n,&o,&a,&idx,4,p,p,0,&sv)&&near(n.position[0],10)&&near(n.position[2],30)); /*halfway 20,20 then hit: o+delta/2*/
 a.counter=90;n=at(40,10,0,0,0);assert(sa3ds_cam_adjust_run(&n,&o,&a,&idx,4,p,p,1,&sv)&&near(n.position[0],2)&&near(n.position[2],38)); /*turning: 10%*/
 printf("Camera adjust 469D10: PASS adjust0 no-op, unported 1-5, 468650 offset carry/halfway, 4686F0 rotation ease, finish->0/0xB, 4684E0 frame split, 462CF0 entries\n");
}
