#include "emerald_coast_collision.h"
#include "emerald_coast_replay.h"
#include "../src/sa1/physics/movement.h"
#include "../src/sa1/physics/steam_flat.h"
#include "../src/sa1/physics/physics_constants.h"
#include "../src/sa1/physics/surface_frame.h"
#include "../src/sa1/physics/steam_spindash_evidence.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned char *load(const char*path,size_t*n){
    FILE*f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long len=ftell(f);assert(len>0);*n=(size_t)len;
    rewind(f);unsigned char*b=malloc(*n);assert(b);assert(fread(b,1,*n,f)==*n);fclose(f);return b;
}
int main(int argc,char**argv){
    assert(argc>=2);size_t n;unsigned char*b=load(argv[1],&n);SA3DSECCollision s;
    if(argc>2&&argv[2][0]=='r'){
        assert(!sa3ds_ec_collision_open(&s,b,n));free(b);puts("Malformed collision package rejected: PASS");return 0;
    }
    assert(sa3ds_ec_collision_open(&s,b,n));
    /* PC2004 center_height=5.4 is independent of ACTION mesh bounds. */
    SA1PlayerState actor={0};actor.pos=(SA1Vec3){10,1,20};actor.ground_normal=(SA1Vec3){0,1,0};
    SA3CMatrix transform;sa3ds_ec_model_transform(&actor,&transform);
    SA1Vec3 foot=sa3ds_ec_task_origin(&actor);
    assert(fabsf(foot.y)<1e-6f);
    assert(fabsf(transform.m[12]-10)<1e-6f&&fabsf(transform.m[13]-5.4f)<1e-6f);
    assert(fabsf(transform.m[14]-20)<1e-6f&&fabsf(transform.m[2]-1)<1e-6f);
    actor.ground_normal=sa1_vec3_normalize((SA1Vec3){0,1,-0.1f});
    actor.pos=sa1_vec3_add((SA1Vec3){0,3.6f,100},actor.ground_normal);
    foot=sa3ds_ec_task_origin(&actor);sa3ds_ec_model_transform(&actor,&transform);
    SA1Vec3 origin={transform.m[12],transform.m[13],transform.m[14]};
    assert(fabsf(foot.y-3.6f)<1e-5f&&fabsf(foot.z-100)<1e-5f);
    assert(fabsf(sa1_vec3_dot(sa1_vec3_sub(origin,foot),actor.ground_normal)-5.4f)<1e-5f);
    SA1Vec3 x={transform.m[0],transform.m[1],transform.m[2]},y={transform.m[4],transform.m[5],transform.m[6]},z={transform.m[8],transform.m[9],transform.m[10]};
    assert(fabsf(sa1_vec3_length(x)-1)<1e-6f&&fabsf(sa1_vec3_length(y)-1)<1e-6f&&fabsf(sa1_vec3_length(z)-1)<1e-6f);
    assert(fabsf(sa1_vec3_dot(x,y))<1e-6f&&fabsf(sa1_vec3_dot(z,y))<1e-6f);
    assert(sa1_vec3_dot(sa1_vec3_cross(x,y),z)>0.99999f); /* no rescaling/reflection */
    printf("Collision: instances=%u triangles=%u grid_refs=%u\n",s.instance_count,s.triangle_count,s.reference_count);
    if(argc==2||argv[2][0]=='o'){
        for(int z=-100;z<=400;z+=50){s.reference_y=0;SA1Surface f=sa3ds_ec_collision_surface(&s,0,(float)z);if(f.valid)printf("floor x=0 z=%d y=%.6f normal=%.3f,%.3f,%.3f source=%u triangle=%u flags=%08x\n",z,f.ground_y,f.normal.x,f.normal.y,f.normal.z,s.triangles[f.surface_id].source_instance,s.triangles[f.surface_id].source_triangle,s.triangles[f.surface_id].flags);}
        float spawn_z=argc>3?strtof(argv[3],NULL):SA3DS_EC_SPAWN_Z;
        assert(sa3ds_ec_collision_spawn(&s,0,spawn_z,0));
        SA1PlayerState p;sa1_player_reset(&p,&s.world);int airborne=0,landed=0;
        for(unsigned t=0;t<600;t++){
            SA1Input in=sa3ds_ec_replay_input(t);SA1Vec3 old=p.pos;s.reference_y=p.pos.y;
            sa1_physics_step_steam_sonic_flat(&p,&in,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
            sa3ds_ec_collision_resolve_walls(&s,old,&p);
            assert(isfinite(p.pos.x)&&isfinite(p.pos.y)&&isfinite(p.pos.z));
            if(t==240){assert(!p.grounded);airborne=1;}
            if(airborne&&t>240&&p.grounded)landed=1;
            if(t%30==0)printf("replay %u p=%.3f,%.3f,%.3f grounded=%d walls=%u\n",t,p.pos.x,p.pos.y,p.pos.z,p.grounded,s.wall_hits);
        }
        assert(airborne&&landed);puts("Owner collision 600-tick controller replay: PASS (port integration, not retail parity)");

        /* Exercise the exact-evidence charge/release state machine on imported
         * support, not only on the synthetic White Zone plane. */
        sa1_player_reset(&p,&s.world);
        SA1Input charge={0};charge.action_pressed=true;charge.action_held=true;
        SA1Vec3 old=p.pos;s.reference_y=p.pos.y;
        sa1_physics_step_steam_sonic_flat(&p,&charge,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
        sa3ds_ec_collision_resolve_walls(&s,old,&p);
        assert(p.grounded&&p.action_state==SA1_ACTION_SPINDASH_CHARGE);
        charge.action_pressed=false;float expected=p.spindash_charge;
        for(unsigned t=0;t<21;t++){
            expected=sa1_sadx_spindash_charge_held_tick(expected);
            old=p.pos;s.reference_y=p.pos.y;
            sa1_physics_step_steam_sonic_flat(&p,&charge,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
            sa3ds_ec_collision_resolve_walls(&s,old,&p);
            assert(p.grounded&&p.action_state==SA1_ACTION_SPINDASH_CHARGE);
        }
        assert(fabsf(p.spindash_charge-expected)<0.00002f);
        SA1Input release={0};release.action_released=true;
        old=p.pos;s.reference_y=p.pos.y;
        sa1_physics_step_steam_sonic_flat(&p,&release,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
        sa3ds_ec_collision_resolve_walls(&s,old,&p);
        assert(p.grounded&&p.action_state==SA1_ACTION_ROLL&&p.ball_form);
        assert(fabsf(sa1_surface_tangent_speed(&p)-expected)<0.001f);
        printf("Owner imported-ground Spin Dash: PASS (charge=%.6f release-speed=%.6f)\n",
            expected,sa1_surface_tangent_speed(&p));
    }else{
        assert(sa3ds_ec_collision_spawn(&s,0,20,1));assert(fabsf(s.world.reset_position.y-1)<0.0001f);
        SA1PlayerState p;sa1_player_reset(&p,&s.world);SA1Input in={0};
        for(int i=0;i<60;i++){s.reference_y=p.pos.y;sa1_physics_step_steam_sonic_flat(&p,&in,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);assert(p.grounded);assert(fabsf(p.pos.y-1)<0.001f);}
        in.jump_pressed=true;in.jump_held=false;s.reference_y=p.pos.y;
        sa1_physics_step_steam_sonic_flat(&p,&in,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);assert(!p.grounded);
        in.jump_pressed=false;int landed=0;for(int i=0;i<120;i++){s.reference_y=p.pos.y;sa1_physics_step_steam_sonic_flat(&p,&in,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);if(p.grounded){landed=1;break;}}assert(landed);
        p.pos=(SA1Vec3){40,1,20};p.vel=(SA1Vec3){20,0,0};sa3ds_ec_collision_resolve_walls(&s,(SA1Vec3){20,1,20},&p);assert(s.wall_hits);assert(fabsf(p.pos.x-27)<0.001f);assert(fabsf(p.vel.x)<0.001f);
        /* A wall leaning over a real floor must not push a grounded actor
         * below that floor after the controller already resolved support. */
        for(uint32_t ti=0;ti<s.triangle_count;ti++){
            SA3DSECTriangle*t=&s.triangles[ti];
            if(fabsf(t->normal.y)<0.1f){
                t->a.x-=t->a.y*0.2f;t->b.x-=t->b.y*0.2f;t->c.x-=t->c.y*0.2f;
                t->normal=sa1_vec3_normalize(sa1_vec3_cross(sa1_vec3_sub(t->b,t->a),sa1_vec3_sub(t->c,t->a)));
            }
        }
        p.pos=(SA1Vec3){40,1,20};p.vel=(SA1Vec3){20,0,0};p.grounded=true;s.reference_y=1;
        sa3ds_ec_collision_resolve_walls(&s,(SA1Vec3){20,1,20},&p);
        assert(fabsf(p.pos.y-1)<0.001f&&p.vel.y>=0&&p.pos.x<27);
        s.reference_y=6;SA1Surface slope=sa3ds_ec_collision_surface(&s,0,100);assert(slope.valid);assert(slope.normal.y>0.9f&&slope.normal.y<1);assert(fabsf(slope.ground_y-3.6f)<0.001f);
        SA1Contact c=sa3ds_ec_collision_contact(&s,(SA1Vec3){0,4.6f,100},(SA1Vec3){0,-1,0},2.35f);assert(c.valid);assert(c.surface_flags==1);
        assert(!sa3ds_ec_collision_surface(&s,600,0).valid);
        assert(!sa3ds_ec_collision_contact(&s,(SA1Vec3){NAN,0,0},(SA1Vec3){0,-1,0},2).valid);
        sa1_player_reset(&p,&s.world);p.pos.x=100;
        s.reference_y=p.pos.y;sa1_physics_step_steam_sonic_flat(&p,&in,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
        assert(!p.grounded); /* Departing imported support is not a heightfield. */
        p.pos.y=-100;s.reference_y=p.pos.y;
        sa1_physics_step_steam_sonic_flat(&p,&in,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
        assert(fabsf(p.pos.x-s.world.reset_position.x)<0.001f);
        assert(fabsf(p.pos.z-s.world.reset_position.z)<0.001f);
        puts("Emerald Coast collision synthetic floor/jump/landing/swept-wall/slope/bounds: PASS");
    }
    sa3ds_ec_collision_destroy(&s);b[n-1]^=1;assert(!sa3ds_ec_collision_open(&s,b,n));free(b);return 0;
}
