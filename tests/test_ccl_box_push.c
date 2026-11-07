#include "ccl_box_push.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static double sine(void*c,uint16_t a){(void)c;if(!(a&0x3fff)){const double v[]={0,1,0,-1};return v[a>>14];}return sin(a*6.2831853071795864769/65536.0);}
static void side(SA3DSCheckpointSweep*b,float x,float z,float ex,float ez){
 float ox,oz;assert(sa3ds_ccl_box_side_push(b,(SA1Vec3){x,20,z},&ox,&oz));
 assert(fabsf(ox-ex)<1e-5f&&fabsf(oz-ez)<1e-5f);
}
int main(void){
 SA3DSCheckpointSweep b={.box_center={0,0,0},.player_radius=5,.player_half_height=5,.player_foot_radius=3,
  .box_x=10,.box_y=10,.box_z=10,.box_yaw=0,.sine=sine};
 /*Faces chosen by the A-B / A+B signs; +X keeps the asymmetric radius*.999+halfX.*/
 side(&b,14,2,5*.999f+10,2);side(&b,-14,2,-15*.999f,2);side(&b,2,14,2,15*.999f);side(&b,2,-14,2,-15*.999f);
 /*Exact diagonal ties follow the source's <=0 branches.*/
 side(&b,13,13,5*.999f+10,13);side(&b,-13,-13,-13,-15*.999f);
 /*Yawed box: the result lies on the expanded face in the box frame.*/
 b.box_yaw=0x2000;b.box_center=(SA1Vec3){100,0,-50};
 float ox,oz;SA1Vec3 s={100+9,20,-50+9};assert(sa3ds_ccl_box_side_push(&b,s,&ox,&oz));
 float sn=(float)sine(0,0x2000),cs=(float)sine(0,0x6000);
 float lx=(ox-100)*cs-(oz+50)*sn,lz=(ox-100)*sn+(oz+50)*cs;
 assert((fabsf(fabsf(lx)-15*.999f)<1e-3f||fabsf(lx-(5*.999f+10))<1e-3f||fabsf(fabsf(lz)-15*.999f)<1e-3f));
 /*Full responses for detected hits.*/
 b.box_yaw=0;b.box_center=(SA1Vec3){0,0,0};SA3DSBoxPushResult r;
 SA3DSCheckpointContact top={.kind=1,.sample={1,12,1}};assert(sa3ds_ccl_box_push(&b,&top,(SA1Vec3){1,7,1},&r));
 assert(r.moved&&r.zero_tilt&&r.status_set==0x20&&r.status_clear==8&&fabsf(r.position.y-(10-.01f))<1e-6f);
 SA3DSCheckpointContact under={.kind=3,.sample={1,-12,1}};assert(sa3ds_ccl_box_push(&b,&under,(SA1Vec3){1,-17,1},&r));
 assert(r.status_set==0x10&&fabsf(r.position.y-((-10+.01f)-10))<1e-5f);
 SA3DSCheckpointContact sidehit={.kind=2,.sample={14,3,2}};assert(sa3ds_ccl_box_push(&b,&sidehit,(SA1Vec3){14,-2,2},&r));
 assert(fabsf(r.position.x-(5*.999f+10))<1e-5f&&r.position.y==-2&&r.position.z==2);
 SA3DSCheckpointContact miss={0};assert(sa3ds_ccl_box_push(&b,&miss,(SA1Vec3){1,2,3},&r)&&!r.moved);
 /*418D80: player receives the box push byte unless inhibited.*/
 SA3DSCCLBody p={0},c={0};SA3DSCCLShape prim={.form=7,.push=0x70},box={.form=3,.push=0x77};
 p.rules.push_receive=0x77;assert(sa3ds_ccl_box_push_enabled(&p,&prim,&c,&box));
 c.rules.push_inhibit=0x70;assert(!sa3ds_ccl_box_push_enabled(&p,&prim,&c,&box));
 puts("CCL box push: PASS 419B50 four faces/+X asymmetry/diagonal ties/yaw frame, 419C90 top/underside/side responses, 418D80 receive/inhibit");
}
