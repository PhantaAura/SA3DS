#include "sonic_pose_metrics.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void u16(uint8_t *d,uint16_t n){d[0]=n&255;d[1]=(n>>8)&255;}
static void u32(uint8_t *d,uint32_t n){for(int i=0;i<4;i++)d[i]=(uint8_t)(n>>(8*i));}
static void f32(uint8_t *d,float n){uint32_t x;memcpy(&x,&n,4);u32(d,x);}
static void section(SA3CCharacter *c,int k,uint32_t count,uint32_t offset,uint32_t stride) {
    c->sections[k].count=count; c->sections[k].offset=offset;
    c->sections[k].stride=stride; c->sections[k].size=count*stride;
}
static void closef(float a,float b){assert(fabsf(a-b)<0.0001f);}
int main(void){
    uint8_t raw[512]={0}; SA3CCharacter c={0};c.data=raw;c.size=sizeof(raw);
    section(&c,0,1,0,48);section(&c,1,1,64,32);
    section(&c,2,1,112,24);section(&c,3,3,160,32);
    section(&c,4,3,288,2);section(&c,5,1,320,32);
    u16(raw,0);u16(raw+2,0xffff);u16(raw+4,0xffff);
    u32(raw+64,0);u32(raw+68,1);
    u16(raw+112+2,0);u32(raw+112+4,0);u32(raw+112+8,3);
    f32(raw+160+0,0);f32(raw+160+4,0);f32(raw+160+8,0);
    f32(raw+192+0,1);f32(raw+192+4,0);f32(raw+192+8,0);
    f32(raw+224+0,0);f32(raw+224+4,2);f32(raw+224+8,0);
    u16(raw+288,0);u16(raw+290,1);u16(raw+292,2);
    SA3CMatrix world[1];sa3c_matrix_identity(world);
    SA3DSPoseMetrics m;
    assert(sa3ds_pose_metrics(&c,0,world,1,&m));
    assert(m.triangles==1 && m.visible_vertices==3 && m.draw_calls==1);
    closef(m.min[0],0);closef(m.max[0],1);
    closef(m.min[1],0);closef(m.max[1],2);
    closef(m.min[2],0);closef(m.max[2],0);
    world[0].m[12]=5;world[0].m[13]=-2;world[0].m[14]=3;
    assert(sa3ds_pose_metrics(&c,0,world,1,&m));
    closef(m.min[0],5);closef(m.max[0],6);
    closef(m.min[1],-2);closef(m.max[1],0);
    closef(m.min[2],3);closef(m.max[2],3);
    closef(m.root[0],5);
    world[0].m[0]=NAN;assert(!sa3ds_pose_metrics(&c,0,world,1,&m));
    assert(!sa3ds_pose_metrics(&c,1,world,1,&m));
    puts("Pass10A synthetic pose metrics: PASS");
    return 0;
}
