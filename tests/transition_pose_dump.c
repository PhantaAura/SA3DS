/* Owner-side diagnostic: emits only numeric transition angles to stdout. */
#include "sa3c_character.h"
#include "sa3c_pose.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static int action(const char*s,unsigned*out){char*end;unsigned long n=strtoul(s,&end,10);if(!*s||*end||n>65535)return 0;*out=(unsigned)n;return 1;}
static int scalar(const char*s,float*out){char*end;*out=strtof(s,&end);return *s&&!*end&&isfinite(*out);}
int main(int argc,char**argv){
    unsigned source,dest;float frame,weight;
    if(argc!=6||!action(argv[2],&source)||!scalar(argv[3],&frame)||!action(argv[4],&dest)||!scalar(argv[5],&weight)||fabsf(frame)>100000||weight<0||weight>1)return 2;
    FILE*f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)){fclose(f);return 2;}
    long size=ftell(f);if(size<272||size>128*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    void*bytes=malloc((size_t)size);if(!bytes){fclose(f);return 2;}
    int read_ok=fread(bytes,1,(size_t)size,f)==(size_t)size;fclose(f);
    SA3CCharacter c;
    if(!read_ok||sa3c_character_open(&c,bytes,(size_t)size)!=SA3C_OK||sa3c_character_validate_references(&c)!=SA3C_OK){free(bytes);return 2;}
    size_t count=c.sections[0].count;
    SA3CTransform*local=calloc(count,sizeof(*local)),*scratch=calloc(count,sizeof(*scratch));
    uint16_t root;int result=2;
    if(local&&scratch&&sa3c_pose_sample_transition(&c,(uint16_t)source,frame,(uint16_t)dest,weight,local,scratch,count,&root)==SA3C_POSE_OK){
        for(size_t i=0;i<count;i++)printf("%zu %.9g %.9g %.9g\n",i,local[i].rotation.x,local[i].rotation.y,local[i].rotation.z);
        result=0;
    }
    free(scratch);free(local);free(bytes);return result;
}
