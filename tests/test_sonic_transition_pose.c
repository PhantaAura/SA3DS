#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "sa3c_character.h"
#include "sa3c_pose.h"

static unsigned char *read_all(const char *path,size_t *size) {
    FILE *f=fopen(path,"rb"); if(!f)return NULL;
    fseek(f,0,SEEK_END); long n=ftell(f); rewind(f);
    unsigned char *p=(unsigned char*)malloc((size_t)n);
    if(!p||fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}
    fclose(f);*size=(size_t)n;return p;
}

int main(int argc,char **argv) {
    assert(argc==2);
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
    puts("PC2004 local transition pose + shortest-BAMS blend: PASS");
    return 0;
}
