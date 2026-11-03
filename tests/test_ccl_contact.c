#include "ccl_contact.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int resolve(void*p,uint32_t id,unsigned*category){(void)p;if(id==1){*category=0;return 1;}if(id==50){*category=3;return 1;}return 0;}
static void init(SA3DSCCLBody*a,SA3DSCCLBody*b,SA3DSCCLShape*as,SA3DSCCLShape*bs){
    sa3ds_ccl_body_init(a,1);sa3ds_ccl_body_init(b,50);a->status=0x100;b->status=0x100;a->outer_flags=0x80;b->outer_flags=0xc0;
    a->broad_radius=10;b->broad_radius=4.5f;a->rules=(SA3DSCCLRules){1,16,32,0x40000};b->rules=(SA3DSCCLRules){2,32,64,0x80000};
    *as=(SA3DSCCLShape){7,6,0xe0,0x420,{0,0,0},4};*bs=(SA3DSCCLShape){0,0x50,0x21,0x400,{5,0,0},4.5f};b->task_position=bs->center;
}
int main(int argc,char**argv){
    if(argc==2){
        FILE*f=fopen(argv[1],"rb");assert(f);unsigned char source[160];assert(fread(source,1,sizeof(source),f)==sizeof(source));fclose(f);
        for(unsigned mode=0;mode<4;mode++){
            SA3DSCCLBody a,b;SA3DSCCLShape as,bs;init(&a,&b,&as,&bs);
            assert(sa3ds_ccl_shape_decode(&as,source,(SA1Vec3){0,0,0}));assert(sa3ds_ccl_shape_decode(&bs,source+48,(SA1Vec3){5,0,0}));
            a.status=mode==1?0x100:0;
            as.center.y=5.4f;bs.center.y=5.4f;b.task_position=bs.center;
            a.rules.damage_receive=source[96];b.rules.damage_receive=source[97];
            a.rules.excluded_attributes=source[100]|((uint32_t)source[101]<<8)|((uint32_t)source[102]<<16)|((uint32_t)source[103]<<24);
            b.rules.excluded_attributes=source[104]|((uint32_t)source[105]<<8)|((uint32_t)source[106]<<16)|((uint32_t)source[107]<<24);
            as.damage=sa3ds_sonic_ccl_damage_byte(as.damage,mode==2?6:0,mode==1?0x100:0,mode==3?0x8000:0,0);
            SA3DSCCLContactResult r;assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r));
            uint32_t secondary=source[116]|((uint32_t)source[117]<<8)|((uint32_t)source[118]<<16)|((uint32_t)source[119]<<24);
            printf("%u %u %u %u %u %u %u %u %u %u %u\n",mode,r.broad_overlap,r.eligible,r.overlap,a.status,b.status,as.damage,bs.damage,a.cached_other_id,b.cached_other_id,sa3ds_sonic_ccl_secondary_attributes(secondary,mode==2?6:0));
        }
        return 0;
    }
    assert(argc==1);SA3DSCCLBody a,b;SA3DSCCLShape as,bs;SA3DSCCLContactResult r;
    assert(sa3ds_sonic_ccl_damage_byte(0xe0,0,0,0,0)==0xe0);
    assert(sa3ds_sonic_ccl_damage_byte(0xe0,14,0x100,0,0)==0xe5);
    assert(sa3ds_sonic_ccl_damage_byte(0xe0,6,0,0,0)==0xed);
    assert(sa3ds_sonic_ccl_damage_byte(0xe0,44,0x100,0,0)==0xee);
    assert(sa3ds_sonic_ccl_damage_byte(0xe0,45,0,0,0)==0xe4);
    assert(sa3ds_sonic_ccl_damage_byte(0xe0,45,0x100,0,0)==0xe5);
    assert(sa3ds_sonic_ccl_damage_byte(0xe0,0,0,0x8000,0)==0xef);
    assert(sa3ds_sonic_ccl_damage_byte(0xe0,0,0,0,0x8000)==0xef);
    assert(sa3ds_sonic_ccl_secondary_attributes(0x382008,0)==0x382018);
    assert(sa3ds_sonic_ccl_secondary_attributes(0x382018,72)==0x382008);
    init(&a,&b,&as,&bs);assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r)&&r.overlap);
    assert((a.status&4)&&!(b.status&4)&&a.outer_flags==0xa1&&b.outer_flags==0xc1&&as.damage==0xf0&&bs.damage==0x21);
    assert(a.hit_count==1&&b.hit_count==1&&b.hits.hits[0].actor_id==1&&b.cached_other_id==1);
    init(&a,&b,&as,&bs);as.damage=sa3ds_sonic_ccl_damage_byte(as.damage,14,0x100,0,0);
    assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r)&&r.overlap);
    assert(!(a.status&4)&&(b.status&4)&&bs.damage==0x31&&as.damage==0xe5);
    SA3DSEnemyDamageInput input={.task_status=b.status,.hits=&b.hits,.resolve=resolve,.explosion_radius=-1,
        .cached_damage_other_valid=1,.cached_owner_is_player0=1,.cached_other_shape_attributes=as.attributes,
        .players={{.present=1,.status=0x100,.character=0}}};
    SA3DSEnemyDamageState enemy={.enemy_flags=0x483};SA3DSEnemyDamageResult result;SA3DSCCLQueryCursor cursor;sa3ds_ccl_query_reset(&cursor);
    assert(sa3ds_enemy_damage_check(&enemy,&input,&cursor,&result)&&result.killed&&result.score_delta==10&&result.bounce_player==0);
    assert(a.direction_x==-1&&a.direction_z==0&&!r.pushed_first&&!r.pushed_second);
    /*Contact list saturation still updates the cached last other/shape.*/
    for(unsigned i=1;i<17;i++)assert(sa3ds_ccl_record_contact(&a,&as,0,&b,&bs,0));
    assert(a.hit_count==16&&b.hit_count==16);
    a.hits.actor_id=71;assert(sa3ds_ccl_record_contact(&a,&as,1,&b,&bs,2));
    assert(b.hit_count==16&&b.cached_other_id==71&&b.cached_self_shape==2&&b.cached_other_shape==1);
    b.hit_flags[0]=0xfff3;b.status=0xff3f;b.outer_flags=0xffff;
    sa3ds_ccl_body_clear(&b,&bs,1);
    assert(b.status==0xff03&&b.outer_flags==0xffc0&&b.hit_flags[0]==0xfff0&&b.hit_count==0&&b.cached_other_id==0&&b.cached_other_shape==-1&&!(bs.damage&0x10));
    assert(b.hits.hits[0].other_shape==-1);
    /*Report suppression is directional; damage decision is independent.*/
    init(&a,&b,&as,&bs);as.attributes|=8;
    assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r)&&r.overlap&&a.hit_count==0&&b.hit_count==1&&(a.status&4));
    init(&a,&b,&as,&bs);bs.center.x=8.5f;b.task_position=bs.center;
    assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r)&&!r.overlap&&r.broad_overlap);
    bs.center.x=nextafterf(8.5f,0);assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r)&&r.overlap);
    init(&a,&b,&as,&bs);as.attributes|=0x10;
    assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r)&&!r.eligible&&!r.overlap);
    init(&a,&b,&as,&bs);as.attributes|=0x80000;
    assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r)&&!r.eligible);
    init(&a,&b,&as,&bs);b.task_position.x=14.5f;bs.center.x=0;
    assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r)&&!r.broad_overlap&&!r.overlap);
    assert(r.task_distance_squared==14.5f*14.5f); /*homing may still collect*/
    init(&a,&b,&as,&bs);a.rules.push_receive=1;bs.push=1;b.rules.push_receive=0;
    assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r)&&r.pushed_first&&!r.pushed_second&&a.task_position.x==-3.5f);
    init(&a,&b,&as,&bs);a.rules.push_receive=1;bs.push=1;b.rules.push_receive=0;
    as.attributes|=0x4000;bs.attributes|=0x4000;as.damage=0xe5;
    assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r)&&!r.pushed_first&&(bs.damage&0x10));
    init(&a,&b,&as,&bs);bs.center=as.center;b.task_position=bs.center;
    assert(sa3ds_ccl_sphere_contact(&a,&as,0,&b,&bs,0,&r)&&a.direction_x==0&&a.direction_z==.1f);
    puts("CCL contact: PASS Sonic source strengths/secondary enable, broad/category/strict sphere path, ordered/saturated hit-cache, directional reports/damage and shared enemy-score-bounce decisions; sphere contact normal/push gates; native scheduling pending");
}
