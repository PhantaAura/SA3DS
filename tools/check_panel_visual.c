#include "panel_visual.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0)return 2;
    unsigned char *bytes=malloc((size_t)n);if(!bytes)return 2;
    if(fread(bytes,1,(size_t)n,f)!=(size_t)n)return 2;
    fclose(f);SA3DSPanelVisual v;if(!sa3ds_panel_visual_open(&v,bytes,(size_t)n))return 1;
    for(unsigned frame=0;frame<8;frame++){
        SA3CMatrix world[3];if(!sa3ds_panel_visual_pose(&v,frame,world))return 1;
        for(unsigned node=0;node<3;node++){
            printf("%u %u",frame,node);
            for(unsigned j=0;j<16;j++)printf(" %.9g",world[node].m[j]);
            putchar('\n');
        }
    }
    free(bytes);return 0;
}
