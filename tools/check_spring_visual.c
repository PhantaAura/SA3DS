#include "spring_visual.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)){fclose(f);return 2;}
    long n=ftell(f);if(n<=0||(unsigned long)n>SA3DS_SPRING_VISUAL_MAX_BYTES){fclose(f);return 2;}
    rewind(f);void *data=malloc((size_t)n);if(!data){fclose(f);return 2;}
    int ok=fread(data,1,(size_t)n,f)==(size_t)n;fclose(f);
    SA3DSSpringVisual visual;
    ok=ok&&sa3ds_spring_visual_open(&visual,data,(size_t)n);
    if(ok){unsigned triangles=0,textures=0;
        for(unsigned i=0;i<6;i++){triangles+=visual.components[i].index_count/3;textures+=visual.components[i].textures.texture_count;}
        printf("Spring owner visual reader: PASS (6 components, %u triangles, %u component textures)\n",triangles,textures);
    }
    free(data);return ok?0:1;
}
