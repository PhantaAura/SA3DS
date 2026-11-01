#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "emerald_coast_route.h"
int main(int argc,char **argv){
    assert(argc==2||argc==3);FILE*f=fopen(argv[1],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);
    void*bytes=malloc((size_t)n);assert(bytes);assert(fread(bytes,1,(size_t)n,f)==(size_t)n);assert(!fclose(f));
    SA3DSECRoute route;
    if(argc==3){assert(!sa3ds_ec_route_open(&route,bytes,(size_t)n));assert(!route.start_count&&!route.edge_count);free(bytes);return 0;}
    assert(sa3ds_ec_route_open(&route,bytes,(size_t)n));free(bytes);
    assert(route.start_count==2&&route.edge_count==1);
    const SA3DSECStart*start=sa3ds_ec_route_start(&route,0);
    assert(start&&start->task_origin.x==10&&start->task_origin.y==20&&start->task_origin.z==30);
    assert(start->task_yaw==0x6000);
    assert(fabsf(sa3ds_ec_route_player_yaw(start->task_yaw)+.7853981634f)<1e-6f);
    assert(sa3ds_ec_route_player_yaw(0x4000)==0);
    assert(!sa3ds_ec_route_start(&route,2));
    uint32_t next=99;
    assert(sa3ds_ec_route_next(&route,0,(SA1Vec3){500,30,-60},&next)&&next==1);
    next=99;assert(!sa3ds_ec_route_next(&route,0,(SA1Vec3){522,30,-60},&next)&&next==99);
    assert(!sa3ds_ec_route_next(&route,0,(SA1Vec3){500,52,-60},&next));
    assert(sa3ds_ec_route_next(&route,0,(SA1Vec3){521.999f,30,-60},&next));
    assert(!sa3ds_ec_route_next(&route,1,(SA1Vec3){500,30,-60},&next));
    assert(!sa3ds_ec_route_next(&route,0,(SA1Vec3){NAN,30,-60},&next));
    assert(!sa3ds_ec_route_next(&route,0,(SA1Vec3){500,30,-60},NULL));
    assert(!sa3ds_ec_route_open(NULL,NULL,0));
    assert(!sa3ds_ec_route_open(&route,NULL,0)&&!route.start_count);
    puts("Emerald Coast route metadata: PASS (owner-free, task scale/yaw, strict3D handoff, finite/bounded/act-reference validation)");
    return 0;
}
