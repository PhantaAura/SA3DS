#include "whale_shape.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void wi(unsigned char*p,uint32_t u){for(unsigned k=0;k<4;k++)p[k]=(unsigned char)(u>>(k*8));}
static void wf(unsigned char*p,float f){uint32_t u;memcpy(&u,&f,4);wi(p,u);}
int main(void){
 unsigned char data[256]={0};
 wi(data+80,0);wi(data+84,128);wi(data+88,3);wi(data+92,152);
 wi(data+96,0);wi(data+100,176);wi(data+104,3);wi(data+108,200);
 for(unsigned k=0;k<3;k++)wf(data+128+k*4,(float)(k+1));
 wf(data+176,1);wf(data+188,1);wf(data+204,1);wf(data+212,-1);
 SA3DSWhaleShape s={.data=data,.size=sizeof(data),.vertices=2};
 s.banks[0]=s.banks[1]=(SA3DSWhaleShapeBank){4,{data+80,2},{data+96,2}};
 float p[2][3],n[2][3];
 assert(sa3ds_whale_shape_sample(&s,0,1.5f,p,n,2));
 assert(p[0][0]==.5f&&p[0][1]==1&&p[0][2]==1.5f);
 assert(fabsf(n[0][0]-.70710678f)<1e-7f&&fabsf(n[0][1]-.70710678f)<1e-7f&&n[0][2]==0);
 assert(n[1][0]==0&&n[1][1]==0&&n[1][2]==1); /*Opposed normals fall back.*/
 assert(sa3ds_whale_shape_sample(&s,0,3.5f,p,n,2));
 assert(p[0][0]==.5f/4294967296.f&&p[0][1]==1.f/4294967296.f&&p[0][2]==1.5f/4294967296.f);
 assert(sa3ds_whale_shape_sample(&s,1,4,p,n,2)&&p[0][0]==1&&p[0][2]==3);
 assert(!sa3ds_whale_shape_sample(&s,2,0,p,n,2));
 assert(!sa3ds_whale_shape_sample(&s,0,-1,p,n,2));
 assert(!sa3ds_whale_shape_sample(&s,0,NAN,p,n,2));
 assert(!sa3ds_whale_shape_sample(&s,0,0,p,n,1));
 SA3DSWhaleShape empty;assert(!sa3ds_whale_shape_open(&empty,data,sizeof(data))&&!empty.data);
 puts("Whale shape: PASS independent channel blend,normal normalization/cancellation,source unsigned last-key denominator,duration wrap and bounds; native drawing separate");
}
