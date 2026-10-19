#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sa3c_pose.h"
static void wr16(uint8_t *p,uint16_t v){p[0]=v&255;p[1]=v>>8;}
static void wr32(uint8_t *p,uint32_t v){for(int i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static void near(float a,float b){assert(fabsf(a-b)<0.0002f);}
int main(void){
 uint8_t nodes[4*48]={0};SA3CCharacter c={0};c.data=nodes;c.size=sizeof(nodes);
 c.sections[0]=(SA3CSection){{'N','O','D','E'},4,0,48,192};
 for(int i=0;i<4;i++){
   wr16(nodes+i*48,0xffff);wr16(nodes+i*48+2,0xffff);wr16(nodes+i*48+4,0xffff);
 }
 wr16(nodes+2,1);wr16(nodes+4,3);wr16(nodes+48+4,2);
 SA3CTransform x[4]={0};SA3CMatrix w[4];
 for(int i=0;i<4;i++)x[i].scale=(SA3CVec3){1,1,1};
 x[0].position=(SA3CVec3){10,0,0};x[1].position=(SA3CVec3){0,2,0};
 x[2].position=(SA3CVec3){0,0,3};x[3].position=(SA3CVec3){-2,0,0};
 assert(sa3c_pose_build_world(&c,0,x,4,w,4)==SA3C_POSE_OK);
 near(w[0].m[12],10);near(w[1].m[12],10);near(w[1].m[13],2);
 near(w[2].m[12],10);near(w[2].m[14],3);
 near(w[3].m[12],-2); /* sibling shares its parent, not earlier child */
 wr32(nodes+2*48+8,1); /* NoTranslate */
 assert(sa3c_pose_build_world(&c,0,x,4,w,4)==SA3C_POSE_OK);near(w[2].m[14],0);
 wr32(nodes+2*48+8,0);wr32(nodes+8,0x10); /* no children */
 memset(w,0,sizeof(w));assert(sa3c_pose_build_world(&c,0,x,4,w,4)==SA3C_POSE_OK);
 near(w[1].m[15],0);near(w[3].m[12],-2); /* sibling remains traversed */
 wr32(nodes+8,0);x[0].rotation=(SA3CVec3){0.6f,0.9f,0.4f};
 assert(sa3c_pose_build_world(&c,0,x,4,w,4)==SA3C_POSE_OK);float xyz=w[0].m[0];
 wr32(nodes+8,0x20);assert(sa3c_pose_build_world(&c,0,x,4,w,4)==SA3C_POSE_OK);
 assert(fabsf(xyz-w[0].m[0])>0.01f); /* alternating rotation path is real */
 puts("Pass11A synthetic hierarchy/flags tests PASS");return 0;
}
