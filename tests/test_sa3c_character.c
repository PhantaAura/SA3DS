#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sa3c_character.h"

static unsigned char *read_all(const char *path,size_t *size){
    FILE *f=fopen(path,"rb"); if(!f)return NULL; fseek(f,0,SEEK_END); long n=ftell(f); rewind(f); if(n<=0){fclose(f);return NULL;}
    unsigned char *p=(unsigned char*)malloc((size_t)n); if(!p){fclose(f);return NULL;} if(fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;} fclose(f); *size=(size_t)n; return p;
}
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s sonic.sa3c\n",argv[0]);return 2;}
    size_t n=0; unsigned char *p=read_all(argv[1],&n); if(!p){fprintf(stderr,"read failed\n");return 2;}
    SA3CCharacter c; SA3CResult r=sa3c_character_open(&c,p,n); if(r!=SA3C_OK){fprintf(stderr,"open: %s\n",sa3c_result_name(r));return 1;}
    r=sa3c_character_validate_references(&c); if(r!=SA3C_OK){fprintf(stderr,"refs: %s\n",sa3c_result_name(r));return 1;}
    if(c.sections[0].count!=223 || c.sections[1].count!=87 || c.sections[6].count!=16 || c.sections[7].count!=149){fprintf(stderr,"count mismatch\n");return 1;}
    printf("native SA3C character reader/reference validation: PASS\n"); free(p); return 0;
}
