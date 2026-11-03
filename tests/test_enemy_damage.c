#include "enemy_damage.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {unsigned category[8];int exists[8];} Actors;
static int resolve(void*p,uint32_t id,unsigned*out){Actors*a=p;if(id>=8||!a->exists[id])return 0;*out=a->category[id];return 1;}
int main(void){
    Actors actors={.category={2,0,1,0},.exists={1,1,1,1}};
    SA3DSCCLHits hits;sa3ds_ccl_hits_reset(&hits,50);
    hits.hits[0]=(SA3DSCCLHit){0,0,0}; /*null other task*/
    hits.hits[1]=(SA3DSCCLHit){0,0,1};hits.hits[2]=(SA3DSCCLHit){0,0,2};hits.hits[3]=(SA3DSCCLHit){0,0,3};
    SA3DSCCLQueryCursor cursor;sa3ds_ccl_query_reset(&cursor);
    assert(sa3ds_ccl_query_category(&cursor,&hits,0,resolve,&actors)==1);
    assert(sa3ds_ccl_query_category(&cursor,&hits,0,resolve,&actors)==3);
    assert(sa3ds_ccl_query_category(&cursor,&hits,0,resolve,&actors)==-1&&cursor.index==4);
    assert(sa3ds_ccl_query_category(&cursor,&hits,1,resolve,&actors)==2); /*selector resets*/
    actors.category[1]=1;
    assert(sa3ds_ccl_query_category(&cursor,&hits,0,resolve,&actors)==3); /*current category*/
    hits.actor_id=51;
    assert(sa3ds_ccl_query_category(&cursor,&hits,1,resolve,&actors)==1); /*actor resets*/
    hits.hits[0].other_shape=-1;sa3ds_ccl_query_reset(&cursor);
    assert(sa3ds_ccl_query_category(&cursor,&hits,0,resolve,&actors)==-1&&cursor.index==0);
    /*Failed scans retain BEGIN index, not the last scanned row.*/
    hits.hits[0]=(SA3DSCCLHit){0,0,2};hits.hits[1].other_shape=-1;sa3ds_ccl_query_reset(&cursor);
    assert(sa3ds_ccl_query_category(&cursor,&hits,0,resolve,&actors)==-1&&cursor.index==0);
    assert(sa3ds_ccl_query_category(&cursor,&hits,0,resolve,&actors)==-1&&cursor.index==1);
    cursor.index=16;assert(sa3ds_ccl_query_category(&cursor,&hits,0,resolve,&actors)==-1&&cursor.index==16);
    /*Reset-to-frame source cursor is explicit and caller-owned.*/
    sa3ds_ccl_hits_reset(&hits,50);hits.hits[0]=(SA3DSCCLHit){0,0,1};actors.category[1]=0;
    SA3DSEnemyDamageState state={.enemy_flags=0x483,.squash_velocity=.2f,.velocity_y=4};
    SA3DSEnemyDamageInput in={.task_status=4,.hits=&hits,.resolve=resolve,.resolve_context=&actors,
        .cached_damage_other_valid=1,.cached_owner_is_player0=1,.explosion_radius=-1,
        .players={{.present=1,.status=0x100,.character=0},{.present=1,.status=0x100,.character=2}}};
    SA3DSEnemyDamageResult out;sa3ds_ccl_query_reset(&cursor);
    assert(sa3ds_enemy_damage_check(&state,&in,&cursor,&out));
    assert(out.killed&&out.score_delta==10&&out.bounce_player==0&&out.bounce_local_velocity.y==2&&out.rumble_request);
    assert(state.squash_velocity==.35f&&state.enemy_flags==0x1083&&state.velocity_y==4);
    /*The cached attacker, not the category-query hit, selects player1.*/
    sa3ds_ccl_query_reset(&cursor);in.cached_owner_is_player0=0;
    assert(sa3ds_enemy_damage_check(&state,&in,&cursor,&out)&&out.bounce_player==1);
    sa3ds_ccl_query_reset(&cursor);in.players[1].status|=1;
    assert(sa3ds_enemy_damage_check(&state,&in,&cursor,&out)&&out.bounce_player==-1&&out.score_delta==10);
    sa3ds_ccl_query_reset(&cursor);in.players[1].status=0x100;in.players[1].character=1;
    assert(sa3ds_enemy_damage_check(&state,&in,&cursor,&out)&&out.bounce_player==-1);
    sa3ds_ccl_query_reset(&cursor);in.players[1].character=3;
    assert(sa3ds_enemy_damage_check(&state,&in,&cursor,&out)&&out.bounce_player==1);
    state.enemy_flags=0x483;sa3ds_ccl_query_reset(&cursor);in.cached_other_shape_attributes=0x1000;
    assert(sa3ds_enemy_damage_check(&state,&in,&cursor,&out)&&out.killed&&out.score_delta==10&&out.bounce_player==-1);
    assert(state.enemy_flags==0x1c83&&state.velocity_y==.7f);
    /*No category0/1 hit still returns death under task status4, but no score.
      Category1 is searched only after category0 fails.*/
    actors.category[1]=2;state.enemy_flags=0x483;sa3ds_ccl_query_reset(&cursor);in.cached_damage_other_valid=0;
    assert(sa3ds_enemy_damage_check(&state,&in,&cursor,&out)&&out.killed&&out.score_delta==0&&state.enemy_flags==0x483);
    actors.category[1]=1;sa3ds_ccl_query_reset(&cursor);in.cached_damage_other_valid=1;
    assert(sa3ds_enemy_damage_check(&state,&in,&cursor,&out)&&out.score_delta==10&&cursor.query_id==0x41c750);
    /*Explosion branch changes no collision cursor, squash or score. Strict
      radius equality does not kill; disabled negative radius ignores center.*/
    in.task_status=0;in.position=(SA1Vec3){3,4,0};in.explosion_center=(SA1Vec3){0,0,0};in.explosion_radius=5;
    SA3DSCCLQueryCursor saved=cursor;SA3DSEnemyDamageState before=state;
    assert(sa3ds_enemy_damage_check(&state,&in,&cursor,&out)&&!out.killed);
    in.explosion_radius=nextafterf(5,INFINITY);
    assert(sa3ds_enemy_damage_check(&state,&in,&cursor,&out)&&out.killed&&!out.score_delta&&out.bounce_player==-1);
    assert(!memcmp(&cursor,&saved,sizeof(cursor))&&!memcmp(&state,&before,sizeof(state)));
    in.explosion_radius=-1;in.explosion_center.x=NAN;
    assert(sa3ds_enemy_damage_check(&state,&in,NULL,&out)&&!out.killed);
    puts("Enemy damage: PASS source16-hit cursor/category priority/cache separation, task damage flags, score/bounce gates, strong attributes and strict explosion range; native CCL generation/execution pending");
}
