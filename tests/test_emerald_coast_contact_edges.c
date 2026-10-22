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

static int same_point(SA1Vec3 a,SA1Vec3 b){
    return sa1_vec3_length(sa1_vec3_sub(a,b))<0.001f;
}
static int shared_edge(const SA3DSECTriangle*a,const SA3DSECTriangle*b,SA1Vec3 out[2]){
    SA1Vec3 av[3]={a->a,a->b,a->c},bv[3]={b->a,b->b,b->c};int n=0;
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++)if(same_point(av[i],bv[j])){
        if(n<2)out[n]=av[i];n++;break;
    }
    return n>=2;
}
static float distance_to_edge(SA1Vec3 p,SA1Vec3 a,SA1Vec3 b){
    SA1Vec3 e=sa1_vec3_sub(b,a);float ee=sa1_vec3_dot(e,e);
    float q=ee>1e-8f?sa1_vec3_dot(sa1_vec3_sub(p,a),e)/ee:0;
    q=fmaxf(0,fminf(1,q));return sa1_vec3_length(sa1_vec3_sub(p,sa1_vec3_add(a,sa1_vec3_scale(e,q))));
}
static int corner_trial(SA3DSECCollision*s,SA1Vec3 middle,SA1Vec3 direction,
    SA1Vec3 a,SA1Vec3 b,float end_scale){
    direction=sa1_vec3_normalize(direction);
    SA1Vec3 old_center=sa1_vec3_add(middle,sa1_vec3_scale(direction,12));
    SA1Vec3 new_center=sa1_vec3_add(middle,sa1_vec3_scale(direction,end_scale));
    SA1PlayerState actor;sa1_player_reset(&actor,&s->world);
    actor.grounded=true;actor.ground_normal=(SA1Vec3){0,1,0};
    actor.pos=sa1_vec3_sub(new_center,(SA1Vec3){0,4,0});actor.vel=sa1_vec3_sub(new_center,old_center);
    sa3ds_ec_collision_resolve_walls(s,sa1_vec3_sub(old_center,(SA1Vec3){0,4,0}),&actor);
    return distance_to_edge(sa1_vec3_add(actor.pos,(SA1Vec3){0,4,0}),a,b)>=4.99f;
}
static SA1Vec3 opposite_vertex(const SA3DSECTriangle*t,SA1Vec3 edge[2]){
    SA1Vec3 v[3]={t->a,t->b,t->c};
    for(unsigned i=0;i<3;i++)if(!same_point(v[i],edge[0])&&!same_point(v[i],edge[1]))return v[i];
    return v[0];
}
static void triangle_edge(const SA3DSECTriangle*t,unsigned e,SA1Vec3 out[2]){
    SA1Vec3 v[3]={t->a,t->b,t->c};out[0]=v[e];out[1]=v[(e+1)%3];
}
static SA1PhysicsWorld geometry_test_world(const SA3DSECCollision*s){
    SA1PhysicsWorld world=s->world;
    world.reset_min_x=world.reset_min_y=world.reset_min_z=-1000000.0f;
    world.reset_max_x=world.reset_max_z=1000000.0f;
    return world;
}
static int inside_collision_bounds(const SA3DSECCollision*s,SA1Vec3 p){
    return p.x>=s->world.reset_min_x&&p.x<=s->world.reset_max_x&&
           p.z>=s->world.reset_min_z&&p.z<=s->world.reset_max_z;
}
static int slope_trial(SA3DSECCollision*s,const SA3DSECTriangle*from,
    const SA3DSECTriangle*to,SA1Vec3 edge[2]){
    SA1Vec3 middle=sa1_vec3_scale(sa1_vec3_add(edge[0],edge[1]),.5f);
    SA1Vec3 a=sa1_vec3_add(sa1_vec3_scale(middle,.98f),sa1_vec3_scale(opposite_vertex(from,edge),.02f));
    SA1Vec3 b=sa1_vec3_add(sa1_vec3_scale(middle,.98f),sa1_vec3_scale(opposite_vertex(to,edge),.02f));
    SA1Vec3 travel=sa1_vec3_scale(sa1_vec3_sub(b,a),1.2f);if(sa1_vec3_length(travel)<.01f)return 1;
    if(!inside_collision_bounds(s,a)||!inside_collision_bounds(s,sa1_vec3_add(a,travel)))return -1;
    SA1PhysicsWorld world=geometry_test_world(s);
    SA1PlayerState p;sa1_player_reset(&p,&world);p.pos=sa1_vec3_add(a,from->normal);
    p.vel=travel;p.grounded=true;p.ground_normal=from->normal;
    SA1SurfaceFrame frame=sa1_surface_frame_make(from->normal,travel);
    p.surface_forward=frame.forward;p.surface_right=frame.right;p.yaw=atan2f(frame.forward.x,frame.forward.z);
    SA1Vec3 previous=p.pos;s->reference_y=p.pos.y;SA1Input input={0};
    sa1_physics_step_steam_sonic_flat(&p,&input,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
    sa3ds_ec_collision_resolve_walls(s,previous,&p);
    if(!p.grounded||!isfinite(p.pos.x)||!isfinite(p.pos.y)||!isfinite(p.pos.z))return 0;
    return sa1_vec3_dot(p.ground_normal,to->normal)>.9f?1:2;
}

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
    unsigned corners=0,corner_failures=0;
    for(uint32_t i=0;i<s.triangle_count;i++)for(uint32_t j=i+1;j<s.triangle_count;j++){
        SA3DSECTriangle*a=&s.triangles[i],*c=&s.triangles[j];SA1Vec3 edge[2];
        if(a->source_instance!=c->source_instance||fabsf(a->normal.y)>.45f||fabsf(c->normal.y)>.45f||
           fabsf(sa1_vec3_dot(a->normal,c->normal))>.92f||!shared_edge(a,c,edge))continue;
        SA1Vec3 middle=sa1_vec3_scale(sa1_vec3_add(edge[0],edge[1]),.5f);
        SA1Vec3 directions[3]={sa1_vec3_add(a->normal,c->normal),
            sa1_vec3_add(sa1_vec3_scale(a->normal,3),c->normal),
            sa1_vec3_add(a->normal,sa1_vec3_scale(c->normal,3))};
        for(unsigned d=0;d<3;d++)for(int side=-1;side<=1;side+=2){
            SA1Vec3 direction=sa1_vec3_scale(directions[d],(float)side);
            if(sa1_vec3_length(direction)<.2f)continue;
            corners++;if(!corner_trial(&s,middle,direction,edge[0],edge[1],1))corner_failures++;
            corners++;if(!corner_trial(&s,middle,direction,edge[0],edge[1],-12))corner_failures++;
        }
    }
    printf("Imported angled wall corners: tested=%u penetrated=%u\n",corners,corner_failures);
    unsigned slopes=0,slope_failures=0;
    for(uint32_t i=0;i<s.triangle_count;i++)for(uint32_t j=i+1;j<s.triangle_count;j++){
        SA3DSECTriangle*a=&s.triangles[i],*c=&s.triangles[j];SA1Vec3 edge[2];
        float ndot=sa1_vec3_dot(a->normal,c->normal);
        if(a->source_instance!=c->source_instance||a->normal.y<=.45f||c->normal.y<=.45f||
           ndot<.9f||ndot>.9999f||!shared_edge(a,c,edge))continue;
        int forward=slope_trial(&s,a,c,edge);if(forward>=0){slopes++;if(!forward){if(slope_failures<12)printf("SLOPE AIR source=%u %u->%u ndot=%.6f\n",a->source_instance,a->source_triangle,c->source_triangle,ndot);slope_failures++;}}
        int reverse=slope_trial(&s,c,a,edge);if(reverse>=0){slopes++;if(!reverse){if(slope_failures<12)printf("SLOPE AIR source=%u %u->%u ndot=%.6f\n",a->source_instance,c->source_triangle,a->source_triangle,ndot);slope_failures++;}}
    }
    printf("Imported adjacent slope transitions: tested=%u false-airborne=%u\n",slopes,slope_failures);
    unsigned ledges=0,ledge_failures=0;
    for(uint32_t i=0;i<s.triangle_count&&ledges<128;i++){
        SA3DSECTriangle*t=&s.triangles[i];if(t->normal.y<=.45f)continue;
        for(unsigned e=0;e<3&&ledges<128;e++){
            SA1Vec3 edge[2];triangle_edge(t,e,edge);int neighbor=0;
            for(uint32_t j=0;j<s.triangle_count&&!neighbor;j++){
                SA1Vec3 other[2];if(i==j||s.triangles[j].normal.y<=.45f)continue;
                neighbor=shared_edge(t,&s.triangles[j],other)&&
                    ((same_point(edge[0],other[0])&&same_point(edge[1],other[1]))||
                     (same_point(edge[0],other[1])&&same_point(edge[1],other[0])));
            }
            if(neighbor)continue;
            SA1Vec3 middle=sa1_vec3_scale(sa1_vec3_add(edge[0],edge[1]),.5f);
            SA1Vec3 inward=sa1_project_on_plane(sa1_vec3_sub(opposite_vertex(t,edge),middle),t->normal);
            if(sa1_vec3_length(inward)<.01f)continue;inward=sa1_vec3_normalize(inward);
            SA1Vec3 start=sa1_vec3_add(middle,sa1_vec3_scale(inward,.1f));
            SA1Vec3 outward=sa1_vec3_scale(inward,-2);SA1Vec3 predicted=sa1_vec3_add(sa1_vec3_add(start,t->normal),outward);
            if(!inside_collision_bounds(&s,start)||!inside_collision_bounds(&s,predicted))continue;
            s.reference_y=predicted.y;
            if(sa3ds_ec_collision_contact(&s,predicted,sa1_vec3_scale(t->normal,-1),2.35f).valid)continue;
            SA1PhysicsWorld world=geometry_test_world(&s);
            SA1PlayerState actor;sa1_player_reset(&actor,&world);actor.pos=sa1_vec3_add(start,t->normal);
            actor.vel=outward;actor.grounded=true;actor.ground_normal=t->normal;actor.surface_id=(int32_t)i;
            SA1SurfaceFrame frame=sa1_surface_frame_make(t->normal,outward);
            actor.surface_forward=frame.forward;actor.surface_right=frame.right;actor.yaw=atan2f(frame.forward.x,frame.forward.z);
            SA1Vec3 previous=actor.pos;s.reference_y=actor.pos.y;SA1Input input={0};
            sa1_physics_step_steam_sonic_flat(&actor,&input,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
            sa3ds_ec_collision_resolve_walls(&s,previous,&actor);s.reference_y=actor.pos.y;
            if(sa3ds_ec_collision_contact(&s,actor.pos,sa1_vec3_scale(t->normal,-1),2.35f).valid)continue;
            ledges++;
            if(actor.grounded)ledge_failures++;
        }
    }
    printf("Imported boundary-ledges: tested=%u retained-support=%u\n",ledges,ledge_failures);
    /* No snap during ascent, below a floor, or beyond finite source bounds. */
    SA1PlayerState p;sa1_player_reset(&p,&s.world);
    p.grounded=false;p.action_state=SA1_ACTION_AIRBORNE;
    p.pos=(SA1Vec3){510,40,510};p.vel=(SA1Vec3){0,10,0};
    sa3ds_ec_collision_resolve_walls(&s,(SA1Vec3){510,30,510},&p);
    assert(!p.grounded&&p.pos.y==40);
    p.pos=(SA1Vec3){510,-40,510};p.vel=(SA1Vec3){0,-16,0};
    sa3ds_ec_collision_resolve_walls(&s,(SA1Vec3){510,-30,510},&p);
    assert(!p.grounded&&p.pos.y==-40);
    if(s.instance_count==1){
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
    assert(tested&&missed==0&&drops&&tunneled==0&&corners&&corner_failures==0&&slope_failures==0&&ledges&&ledge_failures==0);
    return 0;
}
