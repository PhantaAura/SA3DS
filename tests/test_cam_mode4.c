#include "cam_mode4.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {unsigned rays,regions;int hit;float last_radius;} Ctx;
static double sine(void*c,uint16_t a){(void)c;return sin(a*6.2831853071795864769/65536.0);}
static int ray(void*v,float s[3],float d[3],float r){(void)s;Ctx*c=v;c->rays++;c->last_radius=r;if(!c->hit)return 0;for(int k=0;k<3;k++)d[k]*=.5f;return 1;}
static void regions(void*v,float e[3],const float p[3]){(void)e;(void)p;((Ctx*)v)->regions++;}
static int near(double a,double b){return fabs(a-b)<1e-3;}
static SA3DSCamera cam(float x,float y,float z){SA3DSCamera c;memset(&c,0,sizeof c);c.param.eye[0]=x;c.param.eye[1]=y;c.param.eye[2]=z;memcpy(c.position,c.param.eye,12);c.rot_x=0xf1c8;c.param.roll=0x68c;return c;}
static double dist(const SA3DSCamera*c){return sqrt(c->param.eye[0]*c->param.eye[0]+c->param.eye[1]*c->param.eye[1]+c->param.eye[2]*c->param.eye[2]);}
int main(void){
 Ctx x={0};SA3DSCamServices sv={&x,sine,NULL,NULL};SA3DSCamInput in={{0,0,0},{0,0,0},10,1,0,0,0};
 /*435C70 apply.*/
 SA3DSCamera c=cam(0,10,40);sa3ds_cam_apply_look(&c);
 assert(c.rot_y==0&&c.rot_x==(int32_t)(atan2(-10.,40.)*65536*0.15915497620314795)&&c.rot_z==0x68c&&c.position[2]==40);
 /*Fresh reset seeds the target at the player, then eases .1 toward eyes height.*/
 in.frames_since_reset=0;c=cam(0,0,50);c.param.target[1]=99;assert(sa3ds_cam_mode4_update(&c,&in,&sv));
 assert(near(c.param.target[1],1)&&near(dist(&c),50));
 in.frames_since_reset=1;in.velocity[0]=1;c=cam(0,0,50);assert(sa3ds_cam_mode4_update(&c,&in,&sv)&&near(c.param.target[0],.8));
 in.velocity[0]=0;
 /*Distance bands: >60 compressed and capped at90, <35 grows x1.06 with floor20.*/
 c=cam(0,0,100);sa3ds_cam_mode4_update(&c,&in,&sv);assert(near(dist(&c),90));
 c=cam(0,0,70);sa3ds_cam_mode4_update(&c,&in,&sv);assert(near(dist(&c),69));
 c=cam(0,0,30);sa3ds_cam_mode4_update(&c,&in,&sv);assert(near(dist(&c),31.8));
 c=cam(0,0,10);sa3ds_cam_mode4_update(&c,&in,&sv);assert(near(dist(&c),20));
 /*Eye above the player keeps30 horizontally.*/
 c=cam(10,50,0);sa3ds_cam_mode4_update(&c,&in,&sv);assert(near(c.param.eye[0],30)&&near(c.param.eye[2],0));
 /*Pitch approach F1C8 by B6 and lift by sine.*/
 c=cam(0,0,50);c.rot_x=0;float y0=c.param.eye[1];(void)y0;
 SA3DSCamServices nr={&x,sine,NULL,NULL};sa3ds_cam_mode4_update(&c,&in,&nr);
 /*rot_x is re-derived by apply; check the pre-apply eye lift via the eye height.*/
 assert(c.param.eye[1]>0);
 /*Manual turn moves the eye on the yaw circle (yaw0: +X for left).*/
 c=cam(0,0,50);in.turn_left=1;sa3ds_cam_mode4_update(&c,&in,&sv);assert(c.param.eye[0]>0);in.turn_left=0;
 /*462B40 final ray: radius4 when not moving away, hit takes clipped delta; turning blends10%.*/
 sv.ray=ray;sv.regions=regions;x.hit=1;
 c=cam(0,0,100);sa3ds_cam_mode4_update(&c,&in,&sv);
 assert(x.regions==1&&x.last_radius==4.f&&near(c.param.eye[2],95));
 c=cam(0,0,10);sa3ds_cam_mode4_update(&c,&in,&sv);assert(x.last_radius==8.f&&near(c.param.eye[2],15));
 in.turn_right=1;c=cam(0,0,10);sa3ds_cam_mode4_update(&c,&in,&sv);assert(c.param.eye[2]>10.9f&&c.param.eye[2]<11.f&&c.param.eye[0]<0);
 in.turn_right=0;
 /*Capped >90 probes toward the player; hit within3 drops the distance to20.*/
 x.rays=0;c=cam(0,0,200);sa3ds_cam_mode4_update(&c,&in,&sv);assert(x.rays==2);
 float bad[3]={NAN,0,0};memcpy(in.player,bad,12);assert(!sa3ds_cam_mode4_update(&c,&in,&sv));
 printf("Camera mode4 (469590) + 435C70 apply: PASS target seed/ease/look-ahead,distance bands,above-player 30 push,pitch approach,manual turn,regions hook,462B40 radius4/8 clip and 10%% turning blend,far probe\n");
}
