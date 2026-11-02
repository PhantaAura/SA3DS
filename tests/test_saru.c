#include "saru.h"
#include "ring_groups.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static void *load(const char*path,size_t*size){FILE*f=fopen(path,"rb");if(!f)exit(2);fseek(f,0,SEEK_END);long n=ftell(f);if(n<=0||n>SA3DS_SARU_PACKAGE_MAX_BYTES)exit(2);rewind(f);void*b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n)exit(2);fclose(f);*size=(size_t)n;return b;}
int main(int argc,char **argv){
    if(argc==3){
        size_t n,l;void*b=load(argv[1],&n),*lut=load(argv[2],&l);assert(l==4100);
        SA3DSSaruDefinitions defs;assert(sa3ds_saru_definitions_open(&defs,b,n));
        SA3DSRings math={.quarter_lut=lut};
        for(uint32_t i=0;i<defs.placements.count;i++){
            SA3DSMovementPlacement p;assert(sa3ds_movement_objects_placement(&defs.placements,i,&p));
            uint16_t a=(uint16_t)p.angles[0];SA3DSSaruInitial s;
            assert(sa3ds_saru_initialize(&p,12345,sa3ds_ring_group_sin(&math,a),
                sa3ds_ring_group_sin(&math,(uint16_t)(a+0x4000)),&s));
            printf("%u %u %.9g %.9g %.9g %ld %ld %ld %u %u %u %u %.9g\n",p.act,p.source_row,
                s.task_position.x,s.task_position.y,s.task_position.z,(long)s.task_angles[0],
                (long)s.task_angles[1],(long)s.task_angles[2],s.packed_tag_low,s.mode,s.index_seed,s.task_status,s.task_scale.z);
        }
        free(b);free(lut);return 0;
    }
    assert(argc==1);
    SA3DSMovementPlacement p={.type=29,.position={10,20,30},.angles={0,0x4000,-1},.parameters={-2,0,2}};
    SA3DSSaruInitial s;assert(sa3ds_saru_initialize(&p,0,0,1,&s));
    assert(s.task_position.x==12&&s.task_position.y==20&&s.task_position.z==30);
    assert(s.spawn_origin.x==10&&s.task_status==0x2100&&s.mode==0&&s.index_seed==0);
    assert(s.task_angles[0]==0&&s.task_angles[1]==0x4000&&s.task_angles[2]==255&&s.packed_tag_low==255);
    assert(s.enemy_flags==0x483&&s.behavior_flags==0xe0&&s.animation_frame==3);
    assert(s.task_scale.x==1&&s.task_scale.y==1&&s.task_scale.z==2);
    assert(s.parameter_cc==1.6f&&s.parameter_d0==1&&s.parameter_3c==.1f);
    p.angles[0]=0x4000;p.parameters[0]=0;p.parameters[1]=-1;p.parameters[2]=.1f;
    assert(sa3ds_saru_initialize(&p,32767,1,0,&s));
    assert(s.task_position.x==10&&s.task_position.z==29&&s.task_scale.z==1);
    assert(s.task_status==0x100&&s.mode==1&&s.index_seed==19);
    p.parameters[1]=999.999f;assert(sa3ds_saru_initialize(&p,16384,1,0,&s)&&s.mode==0&&s.index_seed==10);
    p.parameters[1]=1000;assert(sa3ds_saru_initialize(&p,0,1,0,&s)&&s.mode==2);
    assert(!sa3ds_saru_initialize(&p,32768,1,0,&s)&&s.task_status==0);
    p.type=30;assert(!sa3ds_saru_initialize(&p,0,1,0,&s));
    puts("E SARU initialization: PASS orbit/packed-angle tags/modes/radius/RNG/source flags; AI/native spawning not connected");
    return 0;
}
