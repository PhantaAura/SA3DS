/* Geometric contact invariants on actual imported triangles; not retail parity. */
#include "emerald_coast_collision.h"
#include "../src/sa1/physics/surface_frame.h"
#include "../src/sa1/physics/movement.h"
#include "../src/sa1/physics/steam_flat.h"
#include "../src/sa1/physics/physics_constants.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char**argv){
    assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);
    void*b=malloc((size_t)n);assert(b&&fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);
    SA3DSECCollision s;assert(sa3ds_ec_collision_open(&s,b,(size_t)n));
    unsigned tested=0,missed=0,drops=0,tunneled=0;
    for(uint32_t i=0;i<s.triangle_count;i++){
        SA3DSECTriangle*t=&s.triangles[i];if(t->normal.y<=0.45f)continue;
        SA1Vec3 center=sa1_vec3_scale(sa1_vec3_add(sa1_vec3_add(t->a,t->b),t->c),1.0f/3);
        if(fabsf(center.x)<400&&fabsf(center.z)<400&&fabsf(center.y)<25){
            SA1PlayerState p;sa1_player_reset(&p,&s.world);
            p.pos=sa1_vec3_add(sa1_vec3_add(center,t->normal),(SA1Vec3){0,12,0});p.vel=(SA1Vec3){0,-16,0};
            p.grounded=false;p.action_state=SA1_ACTION_AIRBORNE;
            SA1Vec3 previous=p.pos;s.reference_y=p.pos.y;SA1Input input={0};
            sa1_physics_step_steam_sonic_flat(&p,&input,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
            sa3ds_ec_collision_resolve_walls(&s,previous,&p);drops++;
            if(!p.grounded){
                if(tunneled<4)printf("DROP MISS tick=1 source=%u/%u y=%.6f -> %.6f floor=%.6f\n",t->source_instance,t->source_triangle,previous.y,p.pos.y,center.y);
                tunneled++;
            }
            /* Crossing the same plane upward must not attach to the floor. */
            sa1_player_reset(&p,&s.world);p.grounded=false;p.action_state=SA1_ACTION_AIRBORNE;
            previous=sa1_vec3_sub(center,(SA1Vec3){0,4,0});
            p.pos=sa1_vec3_add(center,(SA1Vec3){0,4,0});p.vel=(SA1Vec3){0,8,0};
            sa3ds_ec_collision_resolve_walls(&s,previous,&p);assert(!p.grounded);
        }
        SA1Vec3 vertices[]={t->a,t->b,t->c,center};
        for(unsigned j=0;j<4;j++){
            /* Stay strictly inside the source triangle, including near corners. */
            SA1Vec3 hit=sa1_vec3_add(sa1_vec3_scale(vertices[j],0.98f),sa1_vec3_scale(center,0.02f));
            SA1Vec3 p=sa1_vec3_add(hit,t->normal);
            if(fabsf(p.x)>511||fabsf(p.z)>511)continue;
            s.reference_y=p.y;
            SA1Contact c=sa3ds_ec_collision_contact(&s,p,sa1_vec3_scale(t->normal,-1),2.35f);
            tested++;
            if(!c.valid){
                if(missed<8)printf("MISS source=%u/%u sample=%u p=%.6f,%.6f,%.6f normal=%.6f,%.6f,%.6f\n",t->source_instance,t->source_triangle,j,p.x,p.y,p.z,t->normal.x,t->normal.y,t->normal.z);
                missed++;
            }
        }
    }
    printf("Imported support probes: tested=%u missed=%u triangles=%u\n",tested,missed,s.triangle_count);
    printf("Imported 16-unit/tick descent inputs: tested=%u tunneled=%u\n",drops,tunneled);
    /* No snap during ascent, below a floor, or beyond finite source bounds. */
    SA1PlayerState p;sa1_player_reset(&p,&s.world);
    p.grounded=false;p.action_state=SA1_ACTION_AIRBORNE;
    p.pos=(SA1Vec3){510,40,510};p.vel=(SA1Vec3){0,10,0};
    sa3ds_ec_collision_resolve_walls(&s,(SA1Vec3){510,30,510},&p);
    assert(!p.grounded&&p.pos.y==40);
    p.pos=(SA1Vec3){510,-40,510};p.vel=(SA1Vec3){0,-16,0};
    sa3ds_ec_collision_resolve_walls(&s,(SA1Vec3){510,-30,510},&p);
    assert(!p.grounded&&p.pos.y==-40);
    if(s.triangle_count==6){
        p.pos=(SA1Vec3){-10,-3,32};p.vel=(SA1Vec3){0,-16,0};
        sa3ds_ec_collision_resolve_walls(&s,(SA1Vec3){-10,13,32},&p);
        assert(p.grounded&&fabsf(p.pos.y-1)<0.0001f&&fabsf(p.vel.y)<0.0001f);
        assert(p.support_distance==1&&p.surface_flags==1&&p.action_state==SA1_ACTION_GROUNDED);
        p.grounded=false;p.pos=(SA1Vec3){-40,-4,32};p.vel=(SA1Vec3){0,-16,0};
        sa3ds_ec_collision_resolve_walls(&s,(SA1Vec3){-40,12,32},&p);
        assert(!p.grounded&&p.pos.y==-4); /* Outside the finite floor. */
        p.pos=(SA1Vec3){-40,-2,32};p.vel=(SA1Vec3){-9,-3,0};
        sa3ds_ec_collision_resolve_walls(&s,(SA1Vec3){-31,1,32},&p);
        assert(!p.grounded&&p.pos.x==-40&&p.pos.y==-2);
        /* Leave the fixture's finite left ledge through the real controller. */
        sa1_player_reset(&p,&s.world);p.pos=(SA1Vec3){-31,1,32};
        p.vel=(SA1Vec3){-16,0,0};p.yaw=-1.57079632679f;p.grounded=true;
        SA1Vec3 previous=p.pos;s.reference_y=p.pos.y;SA1Input input={0};
        sa1_physics_step_steam_sonic_flat(&p,&input,0,&s.world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
        sa3ds_ec_collision_resolve_walls(&s,previous,&p);
        assert(p.pos.x< -32&&!p.grounded);
        puts("Finite ledge departure: PASS");
    }
    sa3ds_ec_collision_destroy(&s);free(b);
    assert(tested&&missed==0&&drops&&tunneled==0);
    return 0;
}
