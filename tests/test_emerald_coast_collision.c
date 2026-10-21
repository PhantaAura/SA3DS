#include "emerald_coast_collision.h"
#include "emerald_coast_replay.h"
#include "../src/sa1/physics/movement.h"
#include "../src/sa1/physics/steam_flat.h"
#include "../src/sa1/physics/physics_constants.h"
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
    }else{
        assert(sa3ds_ec_collision_spawn(&s,0,20,1));assert(fabsf(s.world.reset_position.y-1)<0.0001f);
        SA1PlayerState p;sa1_player_reset(&p,&s.world);SA1Input in={0};
        for(int i=0;i<60;i++){s.reference_y=p.pos.y;sa1_physics_step_steam_sonic_flat(&p,&in,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);assert(p.grounded);assert(fabsf(p.pos.y-1)<0.001f);}
        in.jump_pressed=true;in.jump_held=false;s.reference_y=p.pos.y;
        sa1_physics_step_steam_sonic_flat(&p,&in,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);assert(!p.grounded);
        in.jump_pressed=false;int landed=0;for(int i=0;i<120;i++){s.reference_y=p.pos.y;sa1_physics_step_steam_sonic_flat(&p,&in,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);if(p.grounded){landed=1;break;}}assert(landed);
        p.pos=(SA1Vec3){40,1,20};p.vel=(SA1Vec3){20,0,0};sa3ds_ec_collision_resolve_walls(&s,(SA1Vec3){20,1,20},&p);assert(s.wall_hits);assert(p.pos.x<=31.001f);assert(fabsf(p.vel.x)<0.001f);
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
