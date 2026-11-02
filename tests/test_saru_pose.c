#include "saru_pose.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void w16(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;}
static void w32(unsigned char*p,unsigned n){for(unsigned i=0;i<4;i++)p[i]=n>>(i*8);}
static void wf(unsigned char*p,float f){unsigned n;memcpy(&n,&f,4);w32(p,n);}
static void fixture(SA3CCharacter*c,unsigned char*b){
    memset(c,0,sizeof(*c));memset(b,0,4096);c->data=b;c->size=4096;
    c->sections[0]=(SA3CSection){{'N','O','D','E'},38,0,48,1824};
    c->sections[7]=(SA3CSection){{'A','N','I','M'},6,1824,24,144};
    c->sections[8]=(SA3CSection){{'C','H','A','N'},2,1968,24,48};
    c->sections[9]=(SA3CSection){{'K','E','Y','S'},4,2016,16,64};
    for(unsigned i=0;i<38;i++){
        unsigned char*n=b+i*48;w16(n,65535);w16(n+2,65535);w16(n+4,65535);
        wf(n+36,1);wf(n+40,1);wf(n+44,1);
    }
    for(unsigned i=0;i<6;i++){
        unsigned char*a=b+1824+i*24;w16(a,i);w16(a+4,8);w16(a+6,2);
        w32(a+8,i==1?1:0);w32(a+12,1);w32(a+16,3);
    }
    unsigned char*ch=b+1968;w16(ch+2,3);w16(ch+8,2);w16(ch+10,1);w32(ch+12,2);
    ch+=24;w16(ch+2,2);w16(ch+10,1);w32(ch+12,3);
    w32(b+2032,7);wf(b+2036,7);
    float unit=6.28318530717958647692f/65536.f;
    wf(b+2052,32760*unit);wf(b+2068,-32760*unit);
}
int main(int argc,char**argv){
    if(argc==7){
        FILE*f=fopen(argv[1],"rb");if(!f)return 2;
        fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<272||n>1024*1024)return 2;
        void*b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n)return 2;fclose(f);
        SA3CCharacter c;SA3CTransform pose[38];uint16_t root;
        SA3DSSaruAnimation s={.current_action=(uint16_t)strtoul(argv[2],0,10),.current_frame=strtof(argv[3],0),
            .pending_action=(uint16_t)strtoul(argv[4],0,10),.pending_frame=strtof(argv[5],0),.blend_weight=strtof(argv[6],0),.enemy_flags=0x200};
        if(sa3c_character_open(&c,b,(size_t)n)!=SA3C_OK||sa3ds_saru_pose_sample(&c,&s,pose,&root)!=SA3C_POSE_OK)return 1;
        for(unsigned i=0;i<38;i++)printf("%u %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g\n",i,pose[i].position.x,pose[i].position.y,pose[i].position.z,pose[i].rotation.x,pose[i].rotation.y,pose[i].rotation.z,pose[i].scale.x,pose[i].scale.y,pose[i].scale.z);
        free(b);return 0;
    }
    assert(argc==1);unsigned char bytes[4096];SA3CCharacter c;fixture(&c,bytes);
    SA3CTransform pose[38];uint16_t root;SA3DSSaruAnimation s={.pending_action=1,.blend_weight=.5f,.enemy_flags=0x200};
    assert(sa3ds_saru_pose_sample(&c,&s,pose,&root)==SA3C_POSE_OK&&root==0);
    assert(pose[0].rotation.x==0); /*raw-angle blend crosses zero, not shortest half-turn*/
    s.blend_weight=.25f;
    assert(sa3ds_saru_pose_sample(&c,&s,pose,&root)==SA3C_POSE_OK);
    float unit=6.28318530717958647692f/65536.f;
    assert(fabsf(pose[0].rotation.x-16380*unit)<1e-6f);
    s.blend_weight=.5f;s.current_frame=7.5f;
    assert(sa3ds_saru_pose_sample(&c,&s,pose,&root)==SA3C_POSE_OK&&pose[0].position.x==1.75f);
    s.current_frame=8.5f;
    assert(sa3ds_saru_pose_sample(&c,&s,pose,&root)==SA3C_POSE_OK&&pose[0].position.x==.25f);
    s.current_frame=0;s.enemy_flags=0;s.blend_weight=0;
    assert(sa3ds_saru_pose_sample(&c,&s,pose,&root)==SA3C_POSE_OK&&pose[0].rotation.x==32760*unit);
    s.blend_weight=NAN;assert(sa3ds_saru_pose_sample(&c,&s,pose,&root)==SA3C_POSE_ERR_ARGUMENT);
    puts("E SARU pose: PASS independent motion wrap, raw-angle blend, sparse wrap sampling and ordinary-action fallback; native AI not connected");
}
