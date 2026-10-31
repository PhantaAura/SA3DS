#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player_view.h"
#include "emerald_coast_collision.h"
#include "steam_flat.h"
#include "steam_input.h"
#include "surface_frame.h"
#include "movement.h"
#include "input_adapter_math.h"
#define PI 3.14159265358979323846f

static SA1Surface flat(void *ctx,float x,float z) {
    (void)ctx;(void)x;(void)z;
    return (SA1Surface){.normal={0,1,0},.valid=true};
}
static const SA1PhysicsWorld world={.query_surface=flat,.reset_position={0,1,0},
    .reset_min_x=-1e6f,.reset_max_x=1e6f,.reset_min_y=-1000,
    .reset_min_z=-1e6f,.reset_max_z=1e6f};
/* Independent RH camera basis, matching Citro3D LookAt(false). */
static SA1Vec3 reference_view(SA1Vec3 p,float camera) {
    SA1Vec3 backward={-sinf(camera),0,-cosf(camera)};
    SA1Vec3 right=sa1_vec3_cross((SA1Vec3){0,1,0},backward);
    return (SA1Vec3){sa1_vec3_dot(p,right),p.y,sa1_vec3_dot(p,backward)};
}
static void facing(const SA1PlayerState *p,float camera) {
    SA3CMatrix model;sa3ds_ec_model_transform(p,&model);
    SA1Vec3 forward={-model.m[0],-model.m[1],-model.m[2]};
    /* -X is the original ACTION model's nose direction. Ground support
       projects physical yaw; no input-dependent presentation snapping. */
    SA1SurfaceFrame frame=sa1_surface_frame_make(p->ground_normal,
        (SA1Vec3){sinf(p->yaw),0,cosf(p->yaw)});
    assert(sa1_vec3_dot(forward,frame.forward)>.99999f);
    if(fabsf(p->ground_normal.y-1)<1e-6f) {
        SA1Vec3 ec=reference_view(forward,camera);
        float angle=sa3ds_view_model_yaw(p->yaw,camera);
        SA1Vec3 white={-cosf(angle),0,sinf(angle)};
        assert(sa1_vec3_length(sa1_vec3_sub(ec,white))<2e-6f);
    }
}
static void tick(SA1PlayerState *p,SA1Input screen,float camera) {
    SA1Input core=sa3ds_view_input(screen);
    sa1_physics_step_steam_sonic_flat(p,&core,camera,&world,
        &SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
    facing(p,camera);
}
static void owner_terrain(const char *path) {
    FILE *file=fopen(path,"rb");assert(file);
    assert(!fseek(file,0,SEEK_END));long length=ftell(file);assert(length>0);
    rewind(file);unsigned char *bytes=malloc((size_t)length);assert(bytes);
    assert(fread(bytes,1,(size_t)length,file)==(size_t)length);assert(!fclose(file));
    SA3DSECCollision stage;assert(sa3ds_ec_collision_open(&stage,bytes,(size_t)length));
    assert(sa3ds_ec_collision_spawn(&stage,0,SA3DS_EC_SPAWN_Z,0));
    for(unsigned cam=0;cam<8;cam++)for(unsigned dir=0;dir<8;dir++) {
        SA1PlayerState p;sa1_player_reset(&p,&stage.world);
        float camera=cam*PI/4, angle=dir*PI/4;
        SA1Input input=sa3ds_view_input((SA1Input){.move_x=sinf(angle),.move_z=cosf(angle)});
        p.yaw=camera;
        for(unsigned t=0;t<20;t++) {
            SA1Vec3 before=p.pos;stage.reference_y=p.pos.y;
            sa1_physics_step_steam_sonic_flat(&p,&input,camera,&stage.world,
                &SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
            sa3ds_ec_collision_resolve_walls(&stage,before,&p);
            facing(&p,camera);
        }
    }
    printf("Owner native view: PASS (64 direction/camera replays x20 ticks on %u imported triangles)\n",stage.triangle_count);
    sa3ds_ec_collision_destroy(&stage);free(bytes);
}
int main(int argc,char **argv) {
    const int dirs[9][2]={{0,0},{0,156},{0,-156},{156,0},{-156,0},
        {156,156},{-156,156},{156,-156},{-156,-156}};
    unsigned cases=0;
    for(unsigned cam=0;cam<8;cam++)for(unsigned d=0;d<9;d++)
    for(unsigned strength=1;strength<=2;strength++) {
        float yaw=cam*PI/4;
        SA3DSCirclePadSample raw=sa3ds_circle_pad_map(
            dirs[d][0]/(int)strength,dirs[d][1]/(int)strength,156);
        SA1Input screen={.move_x=raw.normalized_x,.move_z=raw.normalized_y,
            .device_sample_valid=1,.device_raw_x=raw.raw_x,.device_raw_y=raw.raw_y,
            .device_full_scale=raw.full_scale};
        SA1Input core=sa3ds_view_input(screen);
        SA1Input restored=core;restored.move_x=screen.move_x;
        assert(!memcmp(&restored,&screen,sizeof screen));
        SA1SADXFilteredAnalog a=sa1_sadx_filter_logical_analog(core.move_x,core.move_z);
        if(d) {
            SA1Vec3 desired={sinf(yaw+a.angle_radians),0,cosf(yaw+a.angle_radians)};
            SA1Vec3 view=reference_view(desired,yaw);
            assert(view.x*screen.move_x>=0 && -view.z*screen.move_z>=0);
            SA1Vec3 got=sa3ds_view_point(desired,yaw);
            assert(sa1_vec3_length(sa1_vec3_sub(got,view))<1e-6f);
        }else assert(!a.active);
        SA1PlayerState p;sa1_player_reset(&p,&world);p.yaw=yaw;
        for(unsigned t=0;t<240;t++)tick(&p,screen,yaw);
        if(d) {
            SA1Vec3 v=reference_view(p.vel,yaw);
            /* Numerical symmetry is not assumed: PC2004 signed ADC endpoints
               differ. Final screen velocity must agree in both components. */
            if(screen.move_x)assert(v.x*screen.move_x>0);
            if(screen.move_z)assert(-v.z*screen.move_z>0);
        }
        cases++;
    }
    SA1PlayerState p;sa1_player_reset(&p,&world);
    SA1Input forward={.move_z=1},right={.move_x=1};
    for(unsigned t=0;t<160;t++)tick(&p,forward,0);
    float before=p.yaw;tick(&p,right,0);
    assert(fabsf(p.yaw-before)<PI/2-.01f); /* running turn retains inertia */
    SA1Input reverse={.move_z=-1};
    for(unsigned t=0;t<160;t++)tick(&p,reverse,0);
    SA1Input jump=right;jump.jump_pressed=true;jump.jump_held=true;
    tick(&p,jump,PI/2);assert(!p.grounded);
    for(unsigned t=0;t<15;t++)tick(&p,right,PI/2);
    for(unsigned t=0;t<180;t++)tick(&p,forward,PI/2);
    assert(p.grounded);
    p.action_state=SA1_ACTION_ROLL;p.ball_form=true;p.vel=(SA1Vec3){0,0,0};
    tick(&p,right,PI/2);assert(p.action_state==SA1_ACTION_GROUNDED);
    for(unsigned t=0;t<90;t++)tick(&p,right,PI/2);
    assert(reference_view(p.vel,PI/2).x>0);
    assert(cases==144);
    puts("Native view direction: PASS (144 neutral/partial/full direction-camera cases; running turn, reversal, rotated-camera air steering, landing and Roll exit; both model views)");
    if(argc>1)owner_terrain(argv[1]);
    return 0;
}
