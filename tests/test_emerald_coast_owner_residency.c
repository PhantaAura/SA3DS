/* Optional exact-owner package coverage. No owner coordinates/data compiled in. */
#include "emerald_coast_residency.h"
#include "emerald_coast_visual.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void *read_file(const char *path,size_t *size){
    FILE*f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));
    long n=ftell(f);assert(n>0&&n<64*1024*1024);rewind(f);
    void*p=malloc((size_t)n);assert(p&&fread(p,1,(size_t)n,f)==(size_t)n);
    fclose(f);*size=(size_t)n;return p;
}
int main(int argc,char**argv){
    if(argc!=5){fprintf(stderr,"usage: %s COLLISION ACT0 ACT1 ROUTE\n",argv[0]);return 2;}
    size_t cn,rn;void*cb=read_file(argv[1],&cn),*rb=read_file(argv[4],&rn);
    SA3DSECRoute route;assert(sa3ds_ec_route_open(&route,rb,rn));
    uint32_t total=0,max_triangles=0,max_references=0;
    for(uint32_t act=0;act<2;act++){
        size_t vn;void*vb=read_file(argv[2+act],&vn);SA3DSECVisual v;
        assert(sa3ds_ec_visual_open(&v,vb,vn)==SA3DS_EC_VISUAL_OK);
        const SA3DSECStart*start=sa3ds_ec_route_start(&route,act);assert(start);
        SA3DSECCollision region;
        assert(sa3ds_ec_collision_open_region(&region,cb,cn,act,start->task_origin.x,start->task_origin.z));
        SA1PlayerState player;sa3ds_ec_residency_start(&region,start,&player);
        SA1Vec3 task=sa3ds_ec_task_origin(&player);
        assert(task.x==start->task_origin.x&&task.y==start->task_origin.y&&task.z==start->task_origin.z);
        uint32_t windows=0;
        for(uint32_t i=0;i<v.instance_count;i++){
            SA3DSECInstance instance;assert(sa3ds_ec_instance(&v,i,&instance));
            if(!(instance.surface_flags&1))continue;
            SA1PlayerState before=player;
            assert(sa3ds_ec_residency_recenter(&region,cb,cn,instance.center[0],instance.center[2],&player));
            /* Region loading cannot teleport, reset timing or alter momentum. */
            before.surface_id=player.surface_id;assert(!memcmp(&before,&player,sizeof(player)));
            assert(region.world.context==&region&&region.act_index==act);
            if(region.triangle_count>max_triangles)max_triangles=region.triangle_count;
            if(region.reference_count>max_references)max_references=region.reference_count;
            windows++;
        }
        assert(windows);total+=windows;
        printf("Owner residency act=%u solid-instance windows=%u\n",act,windows);
        sa3ds_ec_collision_destroy(&region);free(vb);
    }
    for(uint32_t i=0;i<route.edge_count;i++){
        const SA3DSECActEdge*e=&route.edges[i];uint32_t next=UINT32_MAX;
        assert(sa3ds_ec_route_next(&route,e->from_act,e->center,&next)&&next==e->to_act);
        SA3DSECCollision region;
        assert(sa3ds_ec_collision_open_region(&region,cb,cn,e->from_act,e->center.x,e->center.z));
        sa3ds_ec_collision_destroy(&region);
    }
    printf("Owner route/residency PASS windows=%u maxTriangles=%u maxReferences=%u (loading coverage, not traversal)\n",total,max_triangles,max_references);
    free(rb);free(cb);return 0;
}
