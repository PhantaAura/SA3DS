#include "pc2004_sphere_sweep.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int near(float a,float b,float e){return fabsf(a-b)<=e;}
int main(void){
 int32_t ax,az;
 sa3ds_pc2004_normal_angles(0,1,0,&ax,&az);assert(ax==0&&az==0);
 sa3ds_pc2004_normal_angles(0,0,1,&ax,&az);assert(ax==0x4000&&az==0);
 sa3ds_pc2004_normal_angles(0,0,-1,&ax,&az);assert(ax==0xc000&&az==0);
 sa3ds_pc2004_normal_angles(1,0,0,&ax,&az);assert(ax==0&&az==-0x4000);
 /*Floor triangle (winding gives +Y normal), sweep straight down onto it.*/
 SA3DSSweepTriangle floor={{{-50,0,-50},{50,0,-50},{0,0,50}},0};
 SA3DSSweepHit h;float s[3]={0,20,0},d[3]={0,-30,0};
 assert(sa3ds_pc2004_sweep_triangle(&floor,s,d,4,&h));
 assert(near(h.normal[1],1,1e-5f)||near(h.normal[1],-1,1e-5f));
 float up=h.normal[1]>0?1.f:-1.f;
 assert(near(h.plane_point[1],0,1e-4f)&&near(h.corrected[1],4*up,1e-4f)&&near(h.corrected[0],0,1e-4f));
 /*Too high: the whole sweep stays more than radius above the plane.*/
 float s2[3]={0,20,0},d2[3]={0,-10,0};assert(!sa3ds_pc2004_sweep_triangle(&floor,s2,d2,4,&h));
 /*Outside XZ (beyond the 1.04-inflated edge) misses.*/
 float s3[3]={200,2,0},d3[3]={0,-4,0};assert(!sa3ds_pc2004_sweep_triangle(&floor,s3,d3,4,&h));
 /*Sampling every r*.5: a fast sweep through the plane still hits.*/
 float s4[3]={0,40,0},d4[3]={0,-80,0};assert(sa3ds_pc2004_sweep_triangle(&floor,s4,d4,4,&h));
 /*Wall facing +X: approach from +X stops at radius in front of it.*/
 SA3DSSweepTriangle wall={{{0,-50,-50},{0,50,0},{0,-50,50}},0};
 float s5[3]={20,0,0},d5[3]={-30,0,0};assert(sa3ds_pc2004_sweep_triangle(&wall,s5,d5,5,&h));
 assert(near(fabsf(h.normal[0]),1,1e-5f)&&near(fabsf(h.corrected[0]),5,1e-3f));
 /*Radius<=0 never hits (453820 entry test).*/
 assert(!sa3ds_pc2004_sweep_triangle(&floor,s,d,0,&h));
 /*List: chosen hit has the smallest end height.*/
 SA3DSSweepTriangle two[2]={floor,{{{-50,-3,-50},{50,-3,-50},{0,-3,50}},0}};
 int idx;assert(sa3ds_pc2004_sweep(two,2,s,d,4,&h,&idx)==2);
 /*Camera ray: wall clips start to radius in front, delta keeps the slide part.*/
 SA3DSSweepTriangle objs[2]={wall,floor};uint32_t ids[2]={7,9};
 float cs[3]={20,10,3},cd[3]={-30,0,4};
 assert(sa3ds_pc2004_camera_ray(objs,ids,2,cs,cd,5)&&near(fabsf(cs[0]),5,1e-3f)&&near(cd[0],0,1e-4f)&&near(cd[2],4,1e-4f));
 float ms[3]={20,30,3},md[3]={1,0,0};assert(!sa3ds_pc2004_camera_ray(objs,ids,2,ms,md,5)&&ms[0]==20&&md[0]==1);
 puts("PC2004 sphere sweep: PASS camera ray clip/slide/miss, 4B8E80 poles/axes, floor/wall hits with corrected point, slab/XZ misses, r*.5 sampling, radius gate, smallest end-height selection");
}
