/* Host-only diagnostic. Reads owner-generated SA3C and prints numbers,
   never exports/copies model geometry or copyrighted resources. */
#include "sa3c_character.h"
#include "sa3c_pose.h"
#include "sonic_pose_metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv) {
    if(argc!=2) { fprintf(stderr,"usage: %s YOUR sonic.sa3c\n",argv[0]);return 2; }
    FILE *f=fopen(argv[1],"rb");if(!f){perror("open");return 1;}
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 1;}
    long len=ftell(f);rewind(f);
    if(len<=0 || len>64*1024*1024){fclose(f);return 1;}
    uint8_t *bytes=malloc((size_t)len);if(!bytes){fclose(f);return 1;}
    if(fread(bytes,1,(size_t)len,f)!=(size_t)len){free(bytes);fclose(f);return 1;}
    fclose(f);
    SA3CCharacter c;
    if(sa3c_character_open(&c,bytes,(size_t)len)!=SA3C_OK ||
       sa3c_character_validate_references(&c)!=SA3C_OK){fprintf(stderr,"invalid SA3C\n");free(bytes);return 1;}
    const size_t n=c.sections[0].count;
    SA3CTransform *local=calloc(n,sizeof(*local));
    SA3CMatrix *world=calloc(n,sizeof(*world));
    if(!local||!world){free(local);free(world);free(bytes);return 1;}
    const uint16_t slots[]={1,3,4,5,6,14,18};
    puts("slot,frame,root,nodecount,triangles,vertices,min_x,min_y,min_z,max_x,max_y,max_z,root_x,root_y,root_z");
    int fail=0;
    for(size_t si=0;si<sizeof(slots)/sizeof(slots[0]);si++) {
        for(unsigned frame=0;frame<=2;frame+=2) {
            uint16_t root=0xFFFF;
            if(sa3c_pose_sample_action(&c,slots[si],(float)frame,0,local,n,&root)!=SA3C_POSE_OK ||
               sa3c_pose_build_world(&c,root,local,n,world,n)!=SA3C_POSE_OK) {fail=1;continue;}
            SA3DSPoseMetrics m;
            if(!sa3ds_pose_metrics(&c,root,world,n,&m)){fail=1;continue;}
            printf("%u,%u,%u,%lu,%u,%u,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                slots[si],frame,root,(unsigned long)n,m.triangles,m.visible_vertices,
                m.min[0],m.min[1],m.min[2],m.max[0],m.max[1],m.max[2],m.root[0],m.root[1],m.root[2]);
        }
    }
    free(local);free(world);free(bytes);
    return fail?1:0;
}
