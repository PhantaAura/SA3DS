#include "saru_actor.h"
#include "ring_groups.h"
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {uint16_t random[16];unsigned rng,damage_calls,floor_calls;int kill_at;float floor;double sine;int source_damage,score;SA3DSEnemyDamageInput damage_input;SA3DSCCLQueryCursor cursor;} Context;
static uint16_t rng(void*p){Context*c=p;assert(c->rng<16);return c->random[c->rng++];}
static double sine(void*p,uint16_t a){return a==0x4000?1.:a==0xc000?-1.:((Context*)p)->sine;}
static int32_t angle(void*p,double y,double x){(void)p;return (int32_t)trunc(atan2(y,x)*65536.*.15915497620314795);}
static double floor_height(void*p,const SA3DSSaruActor*a){(void)a;Context*c=p;c->floor_calls++;return c->floor;}
static int damage(void*p,SA3DSSaruActor*a){Context*c=p;c->damage_calls++;
    if(c->source_damage){SA3DSEnemyDamageResult r;assert(sa3ds_saru_damage_check(a,&c->damage_input,&c->cursor,&r));c->score+=r.score_delta;return r.killed;}
    return c->kill_at>0&&(int)c->damage_calls==c->kill_at;}
static int player_category(void*p,uint32_t id,unsigned*out){(void)p;if(id!=1)return 0;*out=0;return 1;}
static SA3DSSaruServices services(Context*c){return (SA3DSSaruServices){c,sine,angle,rng,floor_height,damage};}
static SA3DSSaruActor actor(unsigned state){return (SA3DSSaruActor){.scale={1,1,40},.status=0x100,.action=(uint8_t)state,.animation={.enemy_flags=0x483,.current_frame=3}};}
static void step(SA3DSSaruActor*a,SA3DSSaruActorInput*in,Context*c,SA3DSSaruActorResult*r){SA3DSSaruServices v=services(c);assert(sa3ds_saru_actor_tick(a,in,&v,r));}
typedef struct {SA3DSRings trig;} OwnerMath;
static double owner_sine(void*p,uint16_t a){return sa3ds_ring_group_sin(&((OwnerMath*)p)->trig,a);}
static uint16_t owner_random(void*p){(void)p;return 12345;}
static int no_damage(void*p,SA3DSSaruActor*a){(void)p;(void)a;return 0;}
static double retain_y(void*p,const SA3DSSaruActor*a){(void)p;return (double)a->position.y-4.;}
static void*load(const char*path,size_t*size){
    FILE*f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0&&n<262144);rewind(f);
    void*p=malloc((size_t)n);assert(p&&fread(p,1,(size_t)n,f)==(size_t)n);fclose(f);*size=(size_t)n;return p;
}
int main(int argc,char**argv){
    if(argc==3){
        size_t n,l;void*data=load(argv[1],&n),*lut=load(argv[2],&l);assert(l==4100);
        SA3DSSaruDefinitions defs;assert(sa3ds_saru_definitions_open(&defs,data,n));OwnerMath math={.trig={.quarter_lut=lut}};
        SA3DSSaruServices v={&math,owner_sine,angle,owner_random,retain_y,no_damage};
        uint16_t frames[]={16,16,16,16,13,7};SA3DSSaruActorInput in={.player_position={100000,100000,100000},.action_frames=frames,.visible=1};
        for(uint32_t i=0;i<defs.placements.count;i++)for(unsigned state=4;state<=6;state+=2){
            SA3DSMovementPlacement p;assert(sa3ds_movement_objects_placement(&defs.placements,i,&p));
            SA3DSSaruActor a;SA3DSSaruActorResult r;assert(sa3ds_saru_actor_init(&a,&p,&v));a.action=(uint8_t)state;a.animation.current_action=3;a.animation.current_frame=0;
            assert(sa3ds_saru_actor_tick(&a,&in,&v,&r));
            printf("%u %u %u %ld %.9g %.9g %.9g %.9g %u\n",p.act,p.source_row,state,(long)a.angles[0],a.position.x,a.position.y,a.position.z,a.animation.current_frame,a.hop_phase);
        }
        free(lut);free(data);return 0;
    }
    assert(argc==1);
    const uint16_t frames[]={16,16,16,16,13,7};
    SA3DSSaruActorInput in={.player_position={1000,0,0},.action_frames=frames,.visible=1};
    SA3DSSaruActor a=actor(0);SA3DSSaruActorResult r;Context c={0};
    SA3DSMovementPlacement placement={.type=29,.position={10,20,30},.parameters={1,0,40}};
    SA3DSSaruServices v=services(&c);c.random[0]=16384;
    assert(sa3ds_saru_actor_init(&a,&placement,&v)&&c.rng==1&&a.wait==10&&a.position.x==50&&a.position.z==30);
    assert(a.counter==0&&a.status==0x100&&a.animation.current_frame==3&&a.animation.enemy_flags==0x483);
    a=actor(0);memset(&c,0,sizeof(c));
    a.counter=5;a.wait=5;a.scale.x=.7f;a.scale.y=1.3f;
    step(&a,&in,&c,&r);assert(a.action==0&&a.counter==6&&c.rng==1&&c.damage_calls==1);
    assert(r.draw&&r.display&&r.enqueue_collision&&r.logic_draw.scale.x==.7f*1.2f);
    assert(a.scale.x!=.7f&&a.idle_phase==0x600); /*squash after captured draw scale*/
    a=actor(0);memset(&c,0,sizeof(c));a.counter=1;c.random[0]=20000;
    step(&a,&in,&c,&r);assert(a.action==3&&a.previous_action==0&&a.counter==1&&c.rng==2);
    a=actor(0);memset(&c,0,sizeof(c));a.counter=1;a.angles[2]=255;c.random[0]=0;c.random[1]=32767;
    step(&a,&in,&c,&r);assert(a.action==1&&a.animation.pending_action==0&&a.animation.blend_rate==.3f&&c.rng==2);
    /*625 is not the close branch;40000 is not the middle branch.*/
    a=actor(0);memset(&c,0,sizeof(c));in.player_position=(SA1Vec3){25,0,0};a.counter=1;a.wait=10;
    step(&a,&in,&c,&r);assert(a.action==7&&a.counter==1&&c.rng==1);
    a=actor(0);memset(&c,0,sizeof(c));in.player_position=(SA1Vec3){200,0,0};a.counter=1;a.wait=10;
    step(&a,&in,&c,&r);assert(a.action==0&&c.rng==1);
    a=actor(1);memset(&c,0,sizeof(c));in.player_position=(SA1Vec3){24,0,0};c.random[0]=32767;
    step(&a,&in,&c,&r);assert(a.action==3&&a.animation.blend_rate==.5f&&c.rng==1);
    /*State2 skips distance arbitration. Source draw can complete a blend
      and immediately run the state handler's frame increment in this tick.*/
    a=actor(2);memset(&c,0,sizeof(c));a.animation.enemy_flags|=0x200;a.animation.blend_weight=1;a.animation.blend_rate=.1f;a.animation.pending_action=5;
    step(&a,&in,&c,&r);assert(a.action==2&&a.animation.current_action==5&&a.animation.current_frame==.28f&&r.logic_draw.animation.blend_weight==1);
    a=actor(3);memset(&c,0,sizeof(c));a.animation.current_frame=15.7f;a.angles[1]=0;
    step(&a,&in,&c,&r);assert(a.action==4&&a.angles[1]==0x400&&a.animation.pending_action==3&&r.dust_bursts==1&&c.rng==1);
    a=actor(5);memset(&c,0,sizeof(c));a.animation.current_frame=15.7f;
    step(&a,&in,&c,&r);assert(a.action==6&&a.angles[1]==0xfc00);
    /*Ordinary logic draw visibility blocks squash/blend, not the AI itself;
      moving helper has no initial visibility test.*/
    in.visible=0;in.player_position=(SA1Vec3){1000,0,0};a=actor(2);memset(&c,0,sizeof(c));a.animation.enemy_flags|=0x200;a.animation.blend_rate=.5f;
    step(&a,&in,&c,&r);assert(!r.draw&&a.animation.blend_weight==0);
    a=actor(4);memset(&c,0,sizeof(c));a.animation.enemy_flags|=0x200;a.animation.blend_rate=.5f;
    step(&a,&in,&c,&r);assert(r.draw&&r.logic_draw.variant==1&&a.animation.blend_weight==.5f);
    a=actor(4);memset(&c,0,sizeof(c));a.status|=0x2000;c.floor=30;in.render_suppressed=1;
    step(&a,&in,&c,&r);assert(!r.draw&&c.floor_calls==1&&a.position.y==34&&a.angles[0]!=0);
    in.render_suppressed=0;in.visible=1;
    /*Mode2 forbids the original throw choice; normal mode selects it.*/
    in.player_position=(SA1Vec3){100,0,0};a=actor(7);memset(&c,0,sizeof(c));a.animation.current_frame=15.9f;
    step(&a,&in,&c,&r);assert(a.action==9&&a.status&0x200&&a.animation.pending_action==4&&c.rng==0);
    assert(r.logic_draw.animation.pending_action==4&&a.animation.blend_weight==.085f);
    a=actor(7);memset(&c,0,sizeof(c));a.mode=2;a.animation.current_frame=15.9f;
    step(&a,&in,&c,&r);assert(a.action==2&&a.animation.pending_action==5&&c.rng==1);
    /*Attack uses saved task position and requests projectile before adding.3.*/
    a=actor(9);memset(&c,0,sizeof(c));a.saved_position=(SA1Vec3){1,2,3};a.animation.current_action=4;a.animation.current_frame=9.5f;a.status|=0x200;
    step(&a,&in,&c,&r);assert(!r.projectile&&a.position.x==1&&a.animation.current_frame>9.5f);
    step(&a,&in,&c,&r);assert(r.projectile&&!(a.status&0x200)&&r.projectile_pose.animation.current_frame==9.8f);
    a=actor(9);memset(&c,0,sizeof(c));a.animation.current_action=4;a.animation.current_frame=12.9f;a.angles[2]=33;
    step(&a,&in,&c,&r);assert(a.action==0&&a.animation.current_frame==11&&a.wait==33&&a.animation.blend_rate==.07f&&r.sound_ids[0]==0x362);
    a=actor(10);memset(&c,0,sizeof(c));a.animation.current_frame=15.9f;c.random[0]=16384;
    step(&a,&in,&c,&r);assert(a.action==10&&r.sound_ids[0]==0x361&&c.rng==1);
    a=actor(10);memset(&c,0,sizeof(c));a.animation.current_frame=15.9f;c.random[0]=16383;c.random[1]=32767;
    step(&a,&in,&c,&r);assert(a.action==0&&a.wait==19&&c.rng==2);
    /*Source conditional plus unconditional damage calls and next-tick death.*/
    a=actor(0);memset(&c,0,sizeof(c));in.collision_flag0=1;c.kill_at=3;
    step(&a,&in,&c,&r);assert(c.damage_calls==3&&a.action==12&&!r.death&&a.death_byte10==1);
    step(&a,&in,&c,&r);assert(r.death&&r.destroy&&!r.display&&!r.enqueue_collision&&c.damage_calls==3);
    in.collision_flag0=0;a=actor(8);memset(&c,0,sizeof(c));a.status|=0x400;a.age=600;
    step(&a,&in,&c,&r);assert(a.status&0x400);step(&a,&in,&c,&r);assert(!(a.status&0x400)&&a.counter==2);
    a=actor(11);a.counter=65535;step(&a,&in,&c,&r);assert(a.counter==0&&!r.draw&&r.display);
    /*Actual shared damage helper at both common-tail call sites, retaining
      its source query cursor and actor enemy fields across calls.*/
    SA3DSCCLHits hits;sa3ds_ccl_hits_reset(&hits,50);hits.hits[0]=(SA3DSCCLHit){0,0,1};
    a=actor(8);a.status|=4;memset(&c,0,sizeof(c));c.source_damage=1;sa3ds_ccl_query_reset(&c.cursor);
    c.damage_input=(SA3DSEnemyDamageInput){.hits=&hits,.resolve=player_category,.cached_damage_other_valid=1,.cached_owner_is_player0=1,.explosion_radius=-1,.players={{.present=1,.status=0x100,.character=0}}};
    in.collision_flag0=1;step(&a,&in,&c,&r);
    assert(a.action==12&&c.damage_calls==2&&c.score==10&&a.animation.enemy_flags==0x1083&&a.squash_velocity==.35f);
    step(&a,&in,&c,&r);assert(r.death&&r.destroy&&c.score==10);
    puts("E SARU actor: PASS source ordinary-state transitions/RNG order, strict ranges, turns/orbit, draw/blend phase, throw/death requests and common tail; shared damage bridge checked; native CCL/projectile execution pending");
}
