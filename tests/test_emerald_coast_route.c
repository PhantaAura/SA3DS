#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emerald_coast_route.h"
int main(int argc,char **argv){
    assert(argc==2||argc==3);FILE*f=fopen(argv[1],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);
    void*bytes=malloc((size_t)n);assert(bytes);assert(fread(bytes,1,(size_t)n,f)==(size_t)n);assert(!fclose(f));
    SA3DSECRoute route;
    if(argc==3&&!strcmp(argv[2],"reject")){assert(!sa3ds_ec_route_open(&route,bytes,(size_t)n));assert(!route.start_count&&!route.edge_count);free(bytes);return 0;}
    assert(sa3ds_ec_route_open(&route,bytes,(size_t)n));free(bytes);
    assert(route.start_count==2&&route.edge_count==1);
    assert(route.has_goal==(argc==3));
    if(route.has_goal){
        assert(route.goal.act_index==1&&route.goal.source_row==99&&route.goal.type==81);
        assert(route.goal.center.x==900&&route.goal.center.y==50&&route.goal.center.z==600);
        assert(route.goal.radius==42&&route.goal.load_distance_squared==160000);
    }
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
