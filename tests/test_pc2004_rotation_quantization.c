/* Source-neutral keys chosen to distinguish source truncation from SA Tools
 * rounding. Exercises the public sampler, not a copied private interpolation. */
#include "sa3c_pose.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void u16(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void u32(uint8_t*p,uint32_t v){for(int i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static void f32(uint8_t*p,float f){uint32_t v;memcpy(&v,&f,4);u32(p,v);}
static void section(SA3CCharacter*c,int i,uint32_t off,uint32_t n,uint32_t stride){c->sections[i].offset=off;c->sections[i].count=n;c->sections[i].stride=stride;c->sections[i].size=n*stride;}
static void bams(SA3CVec3 r,int x,int y,int z){
    const double rad=6.28318530717958647692/65536.;
    assert(fabs((double)r.x/rad-x)<0.1);
    assert(fabs((double)r.y/rad-y)<0.1);
    assert(fabs((double)r.z/rad-z)<0.1);
}
int main(void){
    const double rad=6.28318530717958647692/65536.;
    uint8_t data[256]={0};SA3CCharacter c={0};c.data=data;c.size=sizeof(data);
    section(&c,0,0,1,48);section(&c,7,64,1,16);section(&c,8,96,1,24);section(&c,9,128,2,16);
    u16(data+2,0xffff);u16(data+4,0xffff);for(int i=36;i<48;i+=4)f32(data+i,1);
    u16(data+64+2,0);u16(data+64+4,8);u32(data+64+12,1);
    u16(data+96+2,2);u16(data+96+10,2);u32(data+96+12,0);
    u32(data+128,0);u32(data+144,1);
    const int a[3]={100,-100,1},b[3]={103,-103,-2};
    for(int i=0;i<3;i++){f32(data+132+i*4,(float)(a[i]*rad));f32(data+148+i*4,(float)(b[i]*rad));}
    SA3CTransform local;uint16_t root;
    assert(sa3c_pose_sample_action(&c,0,0.5f,0,&local,1,&root)==SA3C_POSE_OK);
    assert(root==0);bams(local.rotation,101,-101,0);
    assert(sa3c_pose_sample_action(&c,0,0.25f,0,&local,1,&root)==SA3C_POSE_OK);bams(local.rotation,100,-100,0);
    /* Interior nonunit key gap; retain division precision, truncate result. */
    u32(data+144,3);
    assert(sa3c_pose_sample_action(&c,0,0.5f,0,&local,1,&root)==SA3C_POSE_OK);bams(local.rotation,100,-100,0);
    /* Source wraps next index and uses denominator1 for negative time delta. */
    assert(sa3c_pose_sample_action(&c,0,3.5f,0,&local,1,&root)==SA3C_POSE_OK);bams(local.rotation,101,-101,0);
    /* Raw signed key interpolation, not a shortest-angle transition blend. */
    f32(data+132,(float)(32000*rad));f32(data+148,(float)(-32000*rad));u32(data+144,1);
    assert(sa3c_pose_sample_action(&c,0,0.5f,0,&local,1,&root)==SA3C_POSE_OK);assert(fabsf(local.rotation.x)<1e-7f);
    puts("PC2004 rotation sampling: PASS (signed truncation, nonunit keys, source end pair, raw angle path)");
}
