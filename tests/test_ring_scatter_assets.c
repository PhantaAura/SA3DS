#include "ring_scatter_assets.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){
    assert(argc==3);
    for(unsigned n=1;n<3;n++){
        FILE*f=fopen(argv[n],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>0);rewind(f);
        unsigned char*b=malloc((size_t)size);assert(b&&fread(b,1,(size_t)size,f)==(size_t)size);fclose(f);
        SA3DSRingScatterAssets assets;int result=sa3ds_ring_scatter_assets_open(&assets,b,(size_t)size);
        if(n==1){assert(result&&assets.collider);SA3DSCCLShape shape;assert(sa3ds_ccl_shape_decode(&shape,assets.collider,(SA1Vec3){0}));assert(shape.radius==5&&shape.form==0);}
        else assert(!result&&!assets.collider);
        assert(!sa3ds_ring_scatter_assets_open(&assets,b,8193));free(b);
    }
    puts("Scatter assets: PASS borrowed CCI/rules reader,valid fixture/reserved-header rejection and allocation-size cap; synthetic data only");
}
