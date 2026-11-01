#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emerald_coast_collision.h"
#include "movement.h"
#include "steam_flat.h"

static SA1Vec3 offset(SA1Vec3 p,SA1Vec3 origin){return (SA1Vec3){p.x+origin.x,p.y+origin.y,p.z+origin.z};}
static void near(float a,float b,float tolerance){assert(fabsf(a-b)<=tolerance);}
static void near3(SA1Vec3 a,SA1Vec3 b,float tolerance){near(a.x,b.x,tolerance);near(a.y,b.y,tolerance);near(a.z,b.z,tolerance);}
static void empty(const SA3DSECCollision*s){
    assert(!s->triangles&&!s->references&&!s->visited&&!s->triangle_count);
    assert(!s->world.query_surface&&!s->world.query_contact);
}
static SA1PlayerState actor(SA1Vec3 p,SA1Vec3 v,int grounded){
    SA1PlayerState out={0};out.pos=p;out.vel=v;out.grounded=grounded!=0;
    out.action_state=grounded?SA1_ACTION_GROUNDED:SA1_ACTION_AIRBORNE;
    out.ground_normal=(SA1Vec3){0,1,0};out.surface_forward=(SA1Vec3){0,0,1};
    out.surface_right=(SA1Vec3){1,0,0};return out;
}
static void resolve_pair(SA3DSECCollision *base,SA3DSECCollision *region,SA1Vec3 origin,
    SA1Vec3 previous,SA1Vec3 current,SA1Vec3 velocity,int grounded,int expect_grounded){
    SA1PlayerState a=actor(current,velocity,grounded),b=actor(offset(current,origin),velocity,grounded);
    base->reference_y=previous.y;region->reference_y=previous.y+origin.y;
    sa3ds_ec_collision_resolve_walls(base,previous,&a);
    sa3ds_ec_collision_resolve_walls(region,offset(previous,origin),&b);
    near3(offset(a.pos,origin),b.pos,0.003f);near3(a.vel,b.vel,0.003f);
    if(a.grounded!=b.grounded||a.grounded!=expect_grounded)
        fprintf(stderr,"region contact mismatch prev=(%g,%g,%g) current=(%g,%g,%g) origin=(%g,%g,%g) got=%d,%d expected=%d\n",previous.x,previous.y,previous.z,current.x,current.y,current.z,origin.x,origin.y,origin.z,a.grounded,b.grounded,expect_grounded);
    assert(a.grounded==b.grounded&&a.grounded==expect_grounded);
    assert(a.action_state==b.action_state);
}
static void compare_region(SA3DSECCollision *base,SA3DSECCollision *region,SA1Vec3 origin){
    assert(base->triangle_count==region->triangle_count&&base->reference_count==region->reference_count);
    assert(!memcmp(base->cells,region->cells,sizeof base->cells));
    for(uint32_t i=0;i<base->triangle_count;i++){
        SA3DSECTriangle *a=&base->triangles[i],*b=&region->triangles[i];
        near3(offset(a->a,origin),b->a,0.0001f);near3(offset(a->b,origin),b->b,0.0001f);
        near3(offset(a->c,origin),b->c,0.0001f);near3(a->normal,b->normal,0.00001f);
        assert(a->wall_edge_mask==b->wall_edge_mask&&a->support_edge_mask==b->support_edge_mask);
    }
    for(int z=4;z<125;z+=8)for(int x=-24;x<=24;x+=8){
        base->reference_y=20;region->reference_y=20+origin.y;
        SA1Surface a=sa3ds_ec_collision_surface(base,(float)x,(float)z);
        SA1Surface b=sa3ds_ec_collision_surface(region,x+origin.x,z+origin.z);
        assert(a.valid&&b.valid);near(a.ground_y+origin.y,b.ground_y,0.0001f);
        near3(a.normal,b.normal,0.00001f);
        SA1Vec3 pos={x,a.ground_y+1,(float)z};
        SA1Contact ca=sa3ds_ec_collision_contact(base,pos,(SA1Vec3){0,-1,0},2.35f);
        SA1Contact cb=sa3ds_ec_collision_contact(region,offset(pos,origin),(SA1Vec3){0,-1,0},2.35f);
        assert(ca.valid&&cb.valid);near3(offset(ca.point,origin),cb.point,0.003f);
    }
    assert(!sa3ds_ec_collision_surface(region,region->max_x+1,region->center_z).valid);
    assert(!sa3ds_ec_collision_contact(region,(SA1Vec3){region->center_x,1,region->min_z-1},
        (SA1Vec3){0,-1,0},2.35f).valid);
    /* Face wall, finite corner, descending floor crossing, genuine boundary ledge. */
    resolve_pair(base,region,origin,(SA1Vec3){20,1,20},(SA1Vec3){30,1,20},(SA1Vec3){10,0,0},1,1);
    resolve_pair(base,region,origin,(SA1Vec3){20,1,50},(SA1Vec3){36,1,70},(SA1Vec3){16,0,20},1,1);
    resolve_pair(base,region,origin,(SA1Vec3){0,12,20},(SA1Vec3){0,-8,20},(SA1Vec3){0,-20,0},0,1);
    resolve_pair(base,region,origin,(SA1Vec3){0,7.3f,127},(SA1Vec3){0,7.4f,140},(SA1Vec3){0,0,13},1,0);
    /* Run the unchanged physics/contact adapter in translated world space. */
    assert(sa3ds_ec_collision_spawn(base,0,10,0));
    assert(sa3ds_ec_collision_spawn(region,origin.x,origin.z+10,origin.y));
    SA1PlayerState a,b;sa1_player_reset(&a,&base->world);sa1_player_reset(&b,&region->world);
    for(unsigned t=0;t<40;t++){
        SA1Input in={.move_z=1};SA1Vec3 ap=a.pos,bp=b.pos;
        base->reference_y=a.pos.y;region->reference_y=b.pos.y;
        sa1_physics_step_steam_sonic_flat(&a,&in,0,&base->world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
        sa1_physics_step_steam_sonic_flat(&b,&in,0,&region->world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
        sa3ds_ec_collision_resolve_walls(base,ap,&a);sa3ds_ec_collision_resolve_walls(region,bp,&b);
        near3(offset(a.pos,origin),b.pos,0.03f);near3(a.vel,b.vel,0.003f);
        assert(a.grounded==b.grounded);
    }
}
int main(int argc,char **argv){
    assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));
    long length=ftell(f);assert(length>0);rewind(f);void*bytes=malloc((size_t)length);assert(bytes);
    assert(fread(bytes,1,(size_t)length,f)==(size_t)length);assert(!fclose(f));
    SA3DSECCollision base,explicit_default,far,other,act_at_origin;
    assert(sa3ds_ec_collision_open(&base,bytes,(size_t)length));
    assert(sa3ds_ec_collision_open_region(&explicit_default,bytes,(size_t)length,0,0,0));
    assert(base.min_x==-512&&base.max_x==512&&base.min_z==-512&&base.max_z==512);
    assert(base.triangle_count==explicit_default.triangle_count);
    assert(!memcmp(base.triangles,explicit_default.triangles,base.triangle_count*sizeof(*base.triangles)));
    assert(!memcmp(base.cells,explicit_default.cells,sizeof base.cells));
    assert(!memcmp(base.references,explicit_default.references,base.reference_count*sizeof(*base.references)));
    assert(sa3ds_ec_collision_open_region(&far,bytes,(size_t)length,1,8192,-16384));
    assert(far.act_index==1&&far.instance_count==1);
    assert(far.world.reset_min_x==8192-512&&far.world.reset_max_z==-16384+512);
    compare_region(&base,&far,(SA1Vec3){8192,0,-16384});
    assert(sa3ds_ec_collision_open_region(&other,bytes,(size_t)length,2,-32768,24576));
    compare_region(&base,&other,(SA1Vec3){-32768,-10,24576});
    assert(sa3ds_ec_collision_open_region(&act_at_origin,bytes,(size_t)length,1,0,0));
    act_at_origin.reference_y=40;SA1Surface floor=sa3ds_ec_collision_surface(&act_at_origin,0,10);
    assert(floor.valid);near(floor.ground_y,32,0.0001f);
    SA3DSECCollision bad;
    assert(!sa3ds_ec_collision_open_region(&bad,bytes,(size_t)length,UINT32_MAX,0,0));empty(&bad);
    assert(!sa3ds_ec_collision_open_region(&bad,bytes,(size_t)length,2,0,0));empty(&bad);
    const float invalid[]={NAN,INFINITY,-INFINITY,FLT_MAX,-FLT_MAX};
    for(unsigned i=0;i<sizeof invalid/sizeof invalid[0];i++){
        assert(!sa3ds_ec_collision_open_region(&bad,bytes,(size_t)length,0,invalid[i],0));empty(&bad);
        assert(!sa3ds_ec_collision_open_region(&bad,bytes,(size_t)length,0,0,invalid[i]));empty(&bad);
    }
    assert(!sa3ds_ec_collision_open_region(&bad,NULL,0,0,0,0));empty(&bad);
    assert(!sa3ds_ec_collision_open_region(NULL,bytes,(size_t)length,0,0,0));
    sa3ds_ec_collision_destroy(&act_at_origin);sa3ds_ec_collision_destroy(&other);
    sa3ds_ec_collision_destroy(&far);sa3ds_ec_collision_destroy(&explicit_default);
    sa3ds_ec_collision_destroy(&base);free(bytes);
    puts("Collision regions: PASS (default identity, two distant acts, act-specific height, translated floor/contact/wall/edge/ledge/physics, invalid/empty regions)");return 0;
}
