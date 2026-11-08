#include "cam_trigger.h"
#include "sa3c_pose.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static double sine(void*c,uint16_t a){(void)c;if(!(a&0x3fff)){const double v[]={0,1,0,-1};return v[a>>14];}return sin(a*6.2831853071795864769/65536.0);}
static SA3DSCamRecord rec(uint8_t shape,uint16_t rx,uint16_t ry,float c0,float c1,float c2,float e0,float e1,float e2){
 SA3DSCamRecord r={7,1,0,shape,rx,ry,{c0,c1,c2},{e0,e1,e2},{0,0,0}};return r;}
static int in(const SA3DSCamRecord*r,float x,float y,float z,const float*h1,const float*h2){
 float c[3]={x,y,z};int v=-1;assert(sa3ds_cam_trigger_inside(r,c,h1,h2,sine,0,&v));return v;}
static int nearf(float a,float b){return fabsf(a-b)<1e-4f;}
static void region_cases(void){
 SA3DSCamRecord sp={61,0,0,0,0,0,{0,0,0},{10,0,100},{0,0,0}};
 float e[3]={3,4,0};assert(sa3ds_cam_region_sphere_push(&sp,e)&&nearf(e[0],6)&&nearf(e[1],8));
 float f[3]={6,8,0};assert(!sa3ds_cam_region_sphere_push(&sp,f)); /*strict r^2*/
 SA3DSCamRecord bx={61,0,0,2,0,0,{0,0,0},{10,20,30},{0,0,0}};
 float g[3]={8,0,0};assert(sa3ds_cam_region_box_push(&bx,g,sine,0)&&nearf(g[0],10)&&g[1]==0);
 float h[3]={0,-15,0};assert(sa3ds_cam_region_box_push(&bx,h,sine,0)&&nearf(h[1],-20));
 float t[3]={1,15,20};assert(sa3ds_cam_region_box_push(&bx,t,sine,0)&&nearf(t[0],1)&&nearf(t[1],20));
 float tie[3]={2,12,22};assert(sa3ds_cam_region_box_push(&bx,tie,sine,0)&&nearf(tie[0],10)); /*tie x*/
 float z[3]={0,0,0};assert(sa3ds_cam_region_box_push(&bx,z,sine,0)&&nearf(z[0],-10)); /*0 -> negative*/
 float o[3]={11,0,0};assert(!sa3ds_cam_region_box_push(&bx,o,sine,0));
 SA3DSCamRecord ry=bx;ry.rot_y=0x4000;float q[3]={0,0,8};assert(sa3ds_cam_region_box_push(&ry,q,sine,0)&&nearf(q[2],10)&&nearf(q[0],0));
 SA3DSCamRecord gt={61,0,0,1,0,0,{0,0,0},{30,30,0},{0,0,-100}};
 float old[3]={0,0,-2},eye[3]={0,0,3};assert(sa3ds_cam_region_gate_push(&gt,old,eye,sine,0)&&eye[2]==-2);
 float o2[3]={0,0,3},e2[3]={0,0,-2};assert(!sa3ds_cam_region_gate_push(&gt,o2,e2,sine,0)&&e2[2]==-2); /*toward point*/
 float o3[3]={0,0,1},e3[3]={0,0,3};assert(!sa3ds_cam_region_gate_push(&gt,o3,e3,sine,0)); /*same side*/
 float o4[3]={0,0,-2},e4[3]={0,0,60};assert(!sa3ds_cam_region_gate_push(&gt,o4,e4,sine,0)); /*outside radius*/
 SA3DSCamRecord small=gt;small.extent[0]=small.extent[1]=5;
 float o5[3]={6,0,-1},e5[3]={6,0,1};assert(sa3ds_cam_region_gate_push(&small,o5,e5,sine,0)&&e5[2]==-1); /*hit x=6>5, NaN rectangle passes*/
 uint8_t prio[256]={0};prio[61]=4;SA3DSCamRecord list[3]={sp,sp,bx};float w[3]={8,0,0};unsigned reverts=9;
 assert(sa3ds_cam_regions_apply(list,3,prio,w,w,sine,0,&reverts)==1&&reverts==0&&nearf(w[0],10)); /*box (last) first; sphere then strict-misses at r*/
 list[2].mode=4;float w2[3]={3,4,0};assert(sa3ds_cam_regions_apply(list,3,prio,w2,w2,sine,0,&reverts)==1);
 puts("CAM region pushes: PASS 4367B0 sphere strict/push, 436AD0 min-gap box (tie x, 0->neg, rotY), 436850 gate revert/toward/same-side/radius/zero-plane NaN, 437E90/437F20 order + record0 skip + priority4");
}
int main(void){
 region_cases();
 /*Sphere: strict squared radius with +4 player Y.*/
 SA3DSCamRecord s=rec(0,0,0,0,0,0,0,0,100);
 assert(in(&s,9.9f,-4,0,0,0)&&!in(&s,10,-4,0,0,0)&&!in(&s,0,6,0,0,0)&&in(&s,0,5.9f,0,0,0));
 /*Box: inclusive extents tested at history1 (+4), broad from current.*/
 SA3DSCamRecord b=rec(2,0,0,0,0,0,10,5,3);float h[3]={10,1,-3};
 assert(in(&b,0,0,0,h,0));h[0]=10.01f;assert(!in(&b,0,0,0,h,0));h[0]=0;h[1]=1.01f;assert(!in(&b,0,0,0,h,0));
 assert(!in(&b,100,0,0,(float[3]){0,0,0},0)); /*broad uses current position*/
 /*Yawed box agrees with the sa3c Ninja matrix (nj_rotx then nj_roty).*/
 SA3DSCamRecord y=rec(2,0x0800,0x2000,50,10,-20,4,6,8);
 SA3CMatrix m;sa3c_matrix_identity(&m);
 for(int i=0;i<200;i++){
  float l[3]={(float)((i%7)-3)*1.5f,(float)((i%5)-2)*3.1f,(float)((i%11)-5)*1.8f};
  /*world = T*Rx*Ry*local built the same way as the source matrix*/
  double ax=0x0800*6.2831853071795864769/65536.0,ay=0x2000*6.2831853071795864769/65536.0,sx=sin(ax),cx=cos(ax),sy=sin(ay),cy=cos(ay);
  double w[3]={cy*l[0]+0*l[1]+sy*l[2]+50,sy*sx*l[0]+cx*l[1]-cy*sx*l[2]+10,-sy*cx*l[0]+sx*l[1]+cy*cx*l[2]-20};
  float hh[3]={(float)w[0],(float)(w[1]-4),(float)w[2]};
  int expect=fabsf(l[0])<=4&&fabsf(l[1])<=6&&fabsf(l[2])<=8;
  assert(in(&y,50,6,-20,hh,0)==expect);
 }
 /*Gate: segment must cross local z=0 within +-extents.*/
 SA3DSCamRecord g=rec(1,0,0,0,0,0,10,10,0);
 float a1[3]={2,-4,1},a2[3]={2,-4,-1};assert(in(&g,0,0,0,a1,a2));
 float b1[3]={2,-4,1},b2[3]={2,-4,0.5f};assert(!in(&g,0,0,0,b1,b2));
 float c1[3]={12,-4,1},c2[3]={12,-4,-1};assert(!in(&g,0,0,0,c1,c2));
 /*Unknown shape never triggers.*/
 SA3DSCamRecord u=rec(3,0,0,0,0,0,1,1,1);assert(!in(&u,0,-4,0,a1,a2));
 puts("CAM trigger: PASS sphere strict/+4, box inclusive/history/broad, Ninja T*Rx*Ry frame, gate z-crossing, unknown shape");
}
