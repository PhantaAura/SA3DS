#include "checkpoint_assets.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void*readall(const char*path,size_t*n){
    FILE*f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long size=ftell(f);assert(size>0);rewind(f);
    void*p=malloc((size_t)size);assert(p);assert(fread(p,1,(size_t)size,f)==(size_t)size);fclose(f);*n=(size_t)size;return p;
}
int main(int argc,char**argv){
    assert(argc>=2);size_t n;void*bytes=readall(argv[1],&n);SA3DSCheckpointAssets a;
    assert(sa3ds_checkpoint_assets_open(&a,bytes,n));
    int dump=argc==3&&!strcmp(argv[2],"dump");
    if(dump)for(unsigned i=0;i<5;i++){
        SA3DSECMaterial material;SA3TTexture tex;const uint8_t*rgba;
        assert(sa3ds_ec_material(&a.components[i],0,&material));
        assert(sa3t_texture(&a.components[i].textures,material.texture_id,&tex,&rgba)==SA3T_OK);
        printf("TEXTURE %u %u\n",i,tex.global_index);
    }
    const int32_t phase[][2]={{16384,-16384},{16384,-11833},{0,-16384},{9130,32767},{-16384,-32768},{125,-987}};
    for(unsigned i=0;i<a.placements.count;i++){
        SA3DSMovementPlacement p;assert(sa3ds_movement_objects_placement(&a.placements,i,&p));assert((p.type&4095)==19);
        for(unsigned j=0;j<6;j++){
            SA3DSCheckpointResult draw={.draw_post_x=phase[j][0],.draw_post_y=phase[j][1]};SA3CMatrix m[3];
            assert(sa3ds_checkpoint_pose(&a,&p,&draw,m));
            for(unsigned k=0;k<3;k++){
                if(dump)printf("%u %u %u",i,j,k);
                for(unsigned v=0;v<16;v++){assert(isfinite(m[k].m[v]));if(dump)printf(" %.9g",m[k].m[v]);}
                if(dump)putchar('\n');
            }
        }
    }
    if(!dump){
        ((unsigned char*)bytes)[n-1]^=1;assert(!sa3ds_checkpoint_assets_open(&a,bytes,n));assert(!a.placements.count&&!a.collider);
        assert(!sa3ds_checkpoint_assets_open(&a,bytes,SA3DS_CHECKPOINT_MAX_BYTES+1u));
        for(int i=2;i<argc;i++){size_t badsize;void*bad=readall(argv[i],&badsize);assert(!sa3ds_checkpoint_assets_open(&a,bad,badsize));free(bad);}
        puts("Checkpoint assets: PASS borrowed views,post poses,CRC/size/reserved/CCI rejection; synthetic art only");
    }
    free(bytes);
}
