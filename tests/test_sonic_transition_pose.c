#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sa3c_character.h"
#include "sa3c_pose.h"

#define BAMS_TO_RAD (6.28318530717958647692f/65536.f)
static void w16(unsigned char*p,unsigned v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);}
static void w32(unsigned char*p,unsigned v){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static void wf(unsigned char*p,float f){unsigned u;memcpy(&u,&f,4);w32(p,u);}
static void transition_case(int a,int b,int destination,float source_frame,float weight,int expected){
    unsigned char bytes[256]={0};SA3CCharacter c={0};c.data=bytes;c.size=sizeof(bytes);
    c.sections[0]=(SA3CSection){{'N','O','D','E'},1,0,48,48};
    c.sections[7]=(SA3CSection){{'A','N','I','M'},2,64,24,48};
    c.sections[8]=(SA3CSection){{'C','H','A','N'},2,112,24,48};
    c.sections[9]=(SA3CSection){{'K','E','Y','S'},3,160,16,48};
    w16(bytes,65535);w16(bytes+2,65535);w16(bytes+4,65535);
    wf(bytes+36,1);wf(bytes+40,1);wf(bytes+44,1);
    for(unsigned i=0;i<2;i++){
        unsigned char*anim=bytes+64+i*24,*ch=bytes+112+i*24;
        w16(anim+2,0);w16(anim+4,2);w32(anim+8,i);w32(anim+12,1);
        w16(ch,0);w16(ch+2,2);w16(ch+10,i?1:2);w32(ch+12,i?2:0);
    }
    w32(bytes+160,0);wf(bytes+164,(float)a*BAMS_TO_RAD);
    w32(bytes+176,1);wf(bytes+180,(float)b*BAMS_TO_RAD);
    w32(bytes+192,0);wf(bytes+196,(float)destination*BAMS_TO_RAD);
    SA3CTransform out,scratch;uint16_t root;
    assert(sa3c_pose_sample_transition(&c,0,source_frame,1,weight,&out,&scratch,1,&root)==SA3C_POSE_OK);
    assert(root==0);
    if(fabsf(out.rotation.x-(float)expected*BAMS_TO_RAD)>=0.0000002f)
        fprintf(stderr,"transition regression a=%d b=%d destination=%d frame=%g weight=%g expected=%d actualBAMS=%g\n",
            a,b,destination,(double)source_frame,(double)weight,expected,(double)(out.rotation.x/BAMS_TO_RAD));
    assert(fabsf(out.rotation.x-(float)expected*BAMS_TO_RAD)<0.0000002f);
}

static void source_neutral_regressions(void){
    transition_case(0,0,3,0,.5f,1); /* final weighted trunc, not round-away */
    transition_case(0,0,-3,0,.5f,-1);
    transition_case(10,9,10,.5f,0,10); /* capture increment trunc before add */
    transition_case(-10,-9,-10,.5f,0,-10);
    transition_case(32760,32760,-32760,0,.5f,32768); /* seam unwrap */
    transition_case(65537,65537,3,0,0,1); /* capture keys sign-normalized */
    transition_case(0,0,32768,0,.5f,-16384); /* negative half-turn tie */
    transition_case(0,32768,0,.5f,0,-16384); /* capture also takes shortest tie */
    transition_case(0,0,3,0,1,3);
}

static unsigned char *read_all(const char *path,size_t *size) {
    FILE *f=fopen(path,"rb"); if(!f)return NULL;
    fseek(f,0,SEEK_END); long n=ftell(f); rewind(f);
    unsigned char *p=(unsigned char*)malloc((size_t)n);
    if(!p||fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}
    fclose(f);*size=(size_t)n;return p;
}

int main(int argc,char **argv) {
    assert(argc==2);
    source_neutral_regressions();
    size_t size=0; unsigned char *bytes=read_all(argv[1],&size); assert(bytes);
    SA3CCharacter c; assert(sa3c_character_open(&c,bytes,size)==SA3C_OK);
    SA3CTransform out[148],scratch[148]; uint16_t root=0xffff;
    assert(sa3c_pose_sample_transition(&c,1,2.0f,3,0.5f,
        out,scratch,148,&root)==SA3C_POSE_OK);
    assert(root==72);
    assert(fabsf(out[72].position.x-1.0f)<1e-6f);
    /* +170 -> -170 crosses the signed-BAMS seam through +/-180, not zero. */
    assert(fabsf(fabsf(out[72].rotation.x)-3.14159265f)<0.001f);
    assert(sa3c_pose_sample_transition(&c,1,2.0f,3,-0.1f,
        out,scratch,148,&root)==SA3C_POSE_ERR_ARGUMENT);
    free(bytes);
    puts("PC2004 transition pose: PASS (separate retained capture, signed seam/half-turn, final weighted truncation)");
    return 0;
}
