#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emerald_coast_residency.h"
int main(int argc,char **argv){
    assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);
    long n=ftell(f);assert(n>0);rewind(f);void*bytes=malloc((size_t)n);assert(bytes);
    assert(fread(bytes,1,(size_t)n,f)==(size_t)n);fclose(f);
    SA3DSECCollision stage;
    assert(sa3ds_ec_collision_open_region(&stage,bytes,(size_t)n,1,8192,-16384));
    SA3DSECStart start={1,{8192,10,-16374},0x4000};
    SA1PlayerState p;sa3ds_ec_residency_start(&stage,&start,&p);
    assert(p.pos.x==8192&&p.pos.y==11&&p.pos.z==-16374&&p.yaw==0);
    assert(p.surface_forward.x==0&&p.surface_forward.z==1);
    assert(stage.world.reset_position.y==11); /* exact task-height bridge, no ground snap */
    stage.world.reset_min_x=-100000;stage.world.reset_max_x=100000;
    stage.world.reset_min_z=-100000;stage.world.reset_max_z=100000;
    p.pos.y=1;stage.reference_y=1;
    SA1Surface floor=sa3ds_ec_collision_surface(stage.world.context,p.pos.x,p.pos.z);
    assert(floor.valid);p.surface_id=floor.surface_id;p.vel=(SA1Vec3){.5f,2,-3};
    p.yaw=-.4f;p.spindash_charge=7;p.jump_ticks=9;
    SA1PlayerState before=p;SA1Vec3 reset=stage.world.reset_position;
    assert(!sa3ds_ec_residency_needs_recenter(&stage,(SA1Vec3){8192+256,0,-16384}));
    assert(sa3ds_ec_residency_needs_recenter(&stage,(SA1Vec3){8192+257,0,-16384}));
    assert(sa3ds_ec_residency_recenter(&stage,bytes,(size_t)n,8320,-16256,&p));
    assert(stage.world.context==&stage);
    /* A cached recenter must retain exactly the freshly validated geometry,
     * contact edge masks, grid ordering and source identities. */
    SA3DSECCollision fresh;
    assert(stage.archive.package==bytes&&stage.archive.package_size==(size_t)n);
    assert(sa3ds_ec_collision_open_region(&fresh,bytes,(size_t)n,1,8320,-16256));
    assert(stage.triangle_count==fresh.triangle_count);
    assert(stage.reference_count==fresh.reference_count);
    assert(!memcmp(stage.triangles,fresh.triangles,stage.triangle_count*sizeof(*stage.triangles)));
    assert(!memcmp(stage.cells,fresh.cells,sizeof(stage.cells)));
    assert(!memcmp(stage.references,fresh.references,stage.reference_count*sizeof(*stage.references)));
    sa3ds_ec_collision_destroy(&fresh);
    assert(stage.world.reset_min_x==-100000&&stage.world.reset_max_z==100000);
    assert(!memcmp(&reset,&stage.world.reset_position,sizeof reset));
    int32_t surface=p.surface_id;p.surface_id=before.surface_id;
    assert(!memcmp(&before,&p,sizeof p));p.surface_id=surface;
    assert(surface>=0&&stage.triangles[surface].source_instance==1);
    floor=stage.world.query_surface(stage.world.context,p.pos.x,p.pos.z);assert(floor.valid);
    SA3DSECTriangle *triangles=stage.triangles;before=p;
    assert(!sa3ds_ec_residency_recenter(&stage,bytes,(size_t)n,NAN,0,&p));
    assert(stage.triangles==triangles&&!memcmp(&p,&before,sizeof p));
    assert(stage.world.query_surface(stage.world.context,p.pos.x,p.pos.z).valid);
    /* A newly supplied package must still pass integrity validation and a
     * rejection must not discard the live region or player state. */
    unsigned char*bad=malloc((size_t)n);assert(bad);memcpy(bad,bytes,(size_t)n);
    bad[n-1]^=1;
    assert(!sa3ds_ec_residency_recenter(&stage,bad,(size_t)n,8320,-16256,&p));
    assert(stage.triangles==triangles&&!memcmp(&p,&before,sizeof p));
    SA3DSECCollisionArchive rejected;
    assert(!sa3ds_ec_collision_archive_open(&rejected,bad,(size_t)n));
    assert(!rejected.package&&!rejected.collision);
    free(bad);
    sa3ds_ec_collision_destroy(&stage);free(bytes);
    puts("Emerald Coast residency: PASS (source start bridge, moving region, cached/fresh geometry and masks, fresh-package integrity, stable context/source identity/state, atomic rejection)");
    return 0;
}
