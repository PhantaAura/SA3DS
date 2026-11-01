/* Source-neutral PC2004 animated/static evaluator distinction. */
#include "sa3c_pose.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void w16(unsigned char*p,unsigned v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);}
static void w32(unsigned char*p,unsigned v){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(i*8));}
static void wf(unsigned char*p,float f){unsigned u;memcpy(&u,&f,4);w32(p,u);}
static void near(float actual,float expected){assert(fabsf(actual-expected)<0.000002f);}
int main(void){
    unsigned char bytes[96]={0};SA3CCharacter c={0};c.data=bytes;c.size=sizeof(bytes);
    c.sections[0]=(SA3CSection){{'N','O','D','E'},2,0,48,96};
    for(unsigned i=0;i<2;i++){
        unsigned char*n=bytes+i*48;
        w16(n,0); /* visible synthetic model, no HIDE flag */
        w16(n+2,i?65535:1);w16(n+4,65535);
        wf(n+36,1);wf(n+40,1);wf(n+44,1);
    }
    unsigned flags=SA3C_EVAL_UNIT_POS|SA3C_EVAL_UNIT_ANG|SA3C_EVAL_UNIT_SCL|SA3C_EVAL_BREAK;
    w32(bytes+8,flags);wf(bytes+12,2);wf(bytes+48+12,1);
    SA3CTransform local[2],saved[2];SA3CMatrix animated[2],stat[2];
    assert(sa3c_pose_bind(&c,local,2)==SA3C_POSE_OK);
    assert(sa3c_pose_build_world_animated(&c,0,local,2,animated,2)==SA3C_POSE_OK);
    near(animated[0].m[12],2);near(animated[1].m[12],3); /* bind fallback despite UNIT_POS */
    local[0].position=(SA3CVec3){5,0,0};
    local[0].rotation=(SA3CVec3){0,0,1.5707963267948966f};
    local[0].scale=(SA3CVec3){2,3,4};
    memcpy(saved,local,sizeof(local));
    assert(sa3c_pose_build_world_animated(&c,0,local,2,animated,2)==SA3C_POSE_OK);
    near(animated[0].m[12],5);near(animated[0].m[1],2);near(animated[0].m[4],-3);near(animated[0].m[10],4);
    near(animated[1].m[12],5);near(animated[1].m[13],2); /* rotated/scaled limb child; BREAK still traversed */
    assert(memcmp(local,saved,sizeof(local))==0);
    memset(stat,0,sizeof(stat));stat[1].m[12]=123;
    assert(sa3c_pose_build_world(&c,0,local,2,stat,2)==SA3C_POSE_OK);
    near(stat[0].m[0],1);near(stat[0].m[5],1);near(stat[0].m[10],1);near(stat[0].m[12],0);
    near(stat[1].m[12],123); /* static BREAK/UNIT behavior remains intact */
    SA3CMatrix object;sa3c_matrix_build_object(&object,local[0].position,local[0].rotation,local[0].scale,flags);
    near(object.m[0],1);near(object.m[12],0);
    w32(bytes+8,flags|SA3C_EVAL_SKIP);
    assert(sa3c_pose_build_world_animated(&c,0,local,2,animated,2)==SA3C_POSE_OK);
    near(animated[0].m[12],2);near(animated[0].m[0],1);near(animated[1].m[12],3);near(animated[1].m[13],0);
    assert(sa3c_pose_build_world_animated(&c,0,local,1,animated,2)==SA3C_POSE_ERR_RANGE);
    assert(sa3c_pose_build_world_animated(&c,0,local,2,animated,1)==SA3C_POSE_ERR_RANGE);
    puts("Animated world: PASS (UNIT channel overrides, bind fallback, scale, BREAK recursion, SKIP bind, static preservation)");
}
