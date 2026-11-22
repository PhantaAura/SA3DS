#include "automatic_loop.h"
#include "spring_player.h"
#include "steam_flat.h"
#include "movement.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static SA1Surface plane(void *context,float x,float z){
    (void)context;(void)x;(void)z;
    return (SA1Surface){.normal={0,1,0},.valid=true};
}
int main(int argc,char **argv){
    static SA3DSAutomaticLoops loops;
    if(argc==2){
        FILE *f=fopen(argv[1],"rb");assert(f);
        assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);
        void *bytes=malloc((size_t)n);assert(bytes&&fread(bytes,1,(size_t)n,f)==(size_t)n);fclose(f);
        assert(sa3ds_automatic_loops_open(&loops,bytes,(size_t)n));
        printf("Owner loop paths decoded: %u paths, %u points\n",loops.count,loops.point_count);
        assert(!sa3ds_automatic_loops_open(&loops,bytes,(size_t)n-1));free(bytes);
    }
    loops.points[0]=(SA1PC2004LoopPoint){100,{0,0,0}};
    loops.points[1]=(SA1PC2004LoopPoint){100,{100,0,0}};
    loops.points[2]=(SA1PC2004LoopPoint){0,{200,0,0}};
    loops.count=1;loops.point_count=3;
    loops.actors[0]=(SA3DSAutomaticLoopActor){
        .path={loops.points,3,200,0},.minimum={-40,-40,-40},.maximum={240,40,40},.cooldown=60};
    SA1PlayerState p={.pos={10,1,0},.vel={7,0,0},.yaw=1.57079632679f,
        .ground_normal={0,1,0},.grounded=true,.pc2004_action_state=2,
        .pc2004_action_state_valid=true,.pc2004_control_timer=100};
    SA3DSSpringPendingCommand spring={0};SA1Input input={.move_z=1};
    assert(sa3ds_spring_player_begin(&p,&spring,&input)&&input.move_z==0);
    assert(sa3ds_automatic_loops_tick(&loops,0,(SA1Vec3){10,0,0},&p));
    assert(!(p.pc2004_status_flags&0x2000)&&p.pc2004_loop_pending);
    assert(fabsf(p.pc2004_loop_distance-10)<1e-5);
    input=(SA1Input){.move_z=1};
    assert(sa3ds_spring_player_begin(&p,&spring,&input)&&input.move_z==1);
    assert((p.pc2004_status_flags&0x2000)&&!p.pc2004_loop_pending);
    SA1Vec3 v;assert(sa1_pc2004_loop_sample(p.pc2004_loop,150,&v)&&v.x==150);
    assert(sa1_pc2004_loop_sample(p.pc2004_loop,200,&v)&&v.x==200);
    assert(!sa1_pc2004_loop_sample(p.pc2004_loop,200.1f,&v));
    SA1SurfaceFrame f=sa1_surface_frame_make((SA1Vec3){0,1,0},(SA1Vec3){1,0,0});
    float speed=7;SA1Vec3 before=p.pos;
    assert(sa1_pc2004_loop_steer(&p,&f,&speed,5,p.vel));
    assert(p.pc2004_loop_distance==17&&p.pos.x==before.x&&p.pos.y==before.y);
    SA1PlayerState neutral=p;
    SA1PhysicsWorld world={.query_surface=plane,.reset_min_x=-10000,.reset_min_y=-100,
        .reset_min_z=-10000,.reset_max_x=10000,.reset_max_z=10000};
    SA1Input idle={0};float distance=neutral.pc2004_loop_distance;
    sa1_physics_step_steam_sonic_flat(&neutral,&idle,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
    assert(neutral.pc2004_loop_distance==distance);
    SA1PlayerState held=p;SA1Input forward={.move_z=1};
    assert(sa3ds_spring_player_begin(&held,&spring,&forward)&&forward.move_z==1);
    sa1_physics_step_steam_sonic_flat(&held,&forward,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
    assert(held.pc2004_loop_distance>distance&&held.pc2004_world_motion.y<-.8f);
    assert(!sa3ds_automatic_loops_tick(&loops,0,(SA1Vec3){10,0,0},&p));
    p.pc2004_status_flags&=~0x2000u;
    assert(!sa3ds_automatic_loops_tick(&loops,0,(SA1Vec3){10,0,0},&p));
    for(unsigned i=0;i<59;i++)assert(!sa3ds_automatic_loops_tick(&loops,0,(SA1Vec3){10,0,0},&p));
    assert(sa3ds_automatic_loops_tick(&loops,0,(SA1Vec3){10,0,0},&p));
    p.pc2004_action_state=1;p.vel=(SA1Vec3){0};sa1_pc2004_loop_begin(&p,3);
    assert(!(p.pc2004_status_flags&0x2000));
    sa3ds_automatic_loops_reset(&loops,&p);assert(!p.pc2004_loop);
    assert(!sa3ds_automatic_loops_tick(&loops,0,(SA1Vec3){-40,0,0},&p));
    puts("automatic loop: deferred entry, timer exception, sampling, endpoint and cooldown PASS");
}
