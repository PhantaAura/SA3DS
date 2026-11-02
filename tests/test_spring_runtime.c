#include "spring_runtime.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static SA3DSMovementPlacement placement(unsigned kind){
    SA3DSMovementPlacement p={0};p.type=(uint16_t)kind;p.parameters[1]=3;p.parameters[0]=12;
    return p;
}
int main(void){
    SA3DSSpringTrig trig={{0,0,0},{1,1,1}};
    for(unsigned kind=1;kind<=2;kind++){
        SA3DSMovementPlacement p=placement(kind);
        SA3DSSpringActor a;sa3ds_spring_actor_reset(&a);
        SA3DSSpringTickInput in={.collision_player=-1,.player_grounded=1,.player_local_forward=1};
        SA3DSSpringTickResult out;
        assert(sa3ds_spring_actor_tick(&a,&p,&trig,&in,&out));
        assert(a.action==1&&a.cooldown==(kind==2?10:0)&&!out.enqueue_collision&&!out.draw);
        for(unsigned tick=1;tick<=(kind==2?0u:10u);tick++){
            assert(sa3ds_spring_actor_tick(&a,&p,&trig,&in,&out));
            assert(a.cooldown==tick&&!out.enqueue_collision&&out.draw&&!out.launched);
        }
        assert(sa3ds_spring_actor_tick(&a,&p,&trig,&in,&out));
        assert(a.cooldown==11&&out.enqueue_collision);
        in.collision_player=1;
        assert(sa3ds_spring_actor_tick(&a,&p,&trig,&in,&out));
        assert(a.action==2&&a.player_index==1&&!out.enqueue_collision&&!out.launched);
        in.collision_player=-1;
        assert(sa3ds_spring_actor_tick(&a,&p,&trig,&in,&out));
        assert(a.action==1&&a.cooldown==(kind==2?5:0));
        assert(out.launched&&out.player_index==1&&out.launch.command.sonic_action==(kind==2?10:9));
        assert(out.launch.world_velocity.y==8&&out.launch.position.y==8);
        unsigned resumed=0;
        for(unsigned ticks=1;ticks<12;ticks++){
            assert(sa3ds_spring_actor_tick(&a,&p,&trig,&in,&out));
            if(out.enqueue_collision){resumed=ticks;break;}
        }
        assert(resumed==(kind==2?6:11));
        SA3DSSpringActor saved=a;in.paused=1;
        assert(sa3ds_spring_actor_tick(&a,&p,&trig,&in,&out));
        assert(!memcmp(&a,&saved,sizeof(a))&&!out.enqueue_collision&&out.draw);
        in.paused=0;in.skip_update=1;
        assert(sa3ds_spring_actor_tick(&a,&p,&trig,&in,&out));
        assert(!memcmp(&a,&saved,sizeof(a))&&!out.draw);
        in.skip_update=0;a.action=2;saved=a;in.player_grounded=0;
        assert(!sa3ds_spring_actor_tick(&a,&p,&trig,&in,&out));
        assert(!memcmp(&a,&saved,sizeof(a))&&!out.launched);
    }
    /* Physical basis must preserve all SET axes and port side handedness. */
    static const int16_t angles[]={0,0x4000,(int16_t)0x8000,(int16_t)0xc000};
    static const float sine[]={0,1,0,-1},cosine[]={1,0,-1,0};
    for(unsigned x=0;x<4;x++)for(unsigned y=0;y<4;y++)for(unsigned z=0;z<4;z++){
        SA3DSMovementPlacement p=placement(2);
        p.angles[0]=angles[x];p.angles[1]=angles[y];p.angles[2]=angles[z];
        SA3DSSpringTrig t={{sine[x],sine[y],sine[z]},{cosine[x],cosine[y],cosine[z]}};
        SA1SurfaceFrame f;assert(sa3ds_springb_physical_frame(&p,&t,&f)&&f.valid);
        SA3DSSpringLaunch launch;
        assert(sa3ds_spring_launch_prepare(&p,&t,(SA1Vec3){0},1,1,&launch));
        assert(launch.world_velocity.x==f.up.x*8&&launch.world_velocity.y==f.up.y*8&&launch.world_velocity.z==f.up.z*8);
        assert(f.right.x==f.up.y*f.forward.z-f.up.z*f.forward.y);
        assert(f.right.y==f.up.z*f.forward.x-f.up.x*f.forward.z);
        assert(f.right.z==f.up.x*f.forward.y-f.up.y*f.forward.x);
    }
    SA3DSMovementPlacement tilted=placement(2);
    tilted.angles[0]=0x2000;tilted.angles[1]=0x3000;tilted.angles[2]=0x1000;
    SA3DSSpringTrig tilt;
    for(unsigned i=0;i<3;i++){
        float radians=(float)tilted.angles[i]*(6.28318530717958647692f/65536.f);
        tilt.sine[i]=sinf(radians);tilt.cosine[i]=cosf(radians);
    }
    SA1SurfaceFrame tf;assert(sa3ds_springb_physical_frame(&tilted,&tilt,&tf));
    SA3DSSpringLaunch tl;assert(sa3ds_spring_launch_prepare(&tilted,&tilt,(SA1Vec3){0},1,1,&tl));
    assert(fabsf(tl.world_velocity.x-tf.up.x*8)<1e-6f);
    assert(fabsf(tl.world_velocity.y-tf.up.y*8)<1e-6f);
    assert(fabsf(tl.world_velocity.z-tf.up.z*8)<1e-6f);
    assert(fabsf(tf.forward.x*tf.up.x+tf.forward.y*tf.up.y+tf.forward.z*tf.up.z)<1e-6f);

    SA3DSSpringCommand command={8,12,3,9,16};SA3DSSpringCommandResult consumed;
    assert(sa3ds_spring_command_consume(&command,0x3503,7,&consumed));
    assert(consumed.sonic_action==9&&consumed.animation_request==16);
    assert(consumed.task_status==0x1003&&consumed.control_timer==7);
    command.command=4;
    assert(sa3ds_spring_command_consume(&command,0x3503,7,&consumed));
    assert(consumed.sonic_action==10&&consumed.control_timer==12&&consumed.task_status==0x1003);
    /* Command4 only clears0x400 with ball0x100 set; low ground bits survive. */
    assert(sa3ds_spring_command_consume(&command,0x1403,7,&consumed));
    assert(consumed.task_status==0x1403);
    assert(!sa3ds_spring_command_consume(&command,3,7,&consumed));
    for(int timer=-2;timer<=2;timer++){
        SA1Input in={.move_x=.7f,.move_z=-.3f,.jump_pressed=true,.device_raw_x=101};
        SA1Input out=sa3ds_spring_filter_analog(in,(int16_t)timer);
        assert(out.jump_pressed&&out.device_raw_x==101);
        assert(out.move_x==(timer?0:.7f));assert(out.move_z==(timer?0:-.3f));
        assert(sa3ds_spring_control_timer_tail((int16_t)timer)==(timer>0?timer-1:0));
    }
    SA3DSSpringActionResult result;
    const SA1SteamPlayerParameters *par=&SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    assert(sa3ds_spring_action_select(9,16,1,20,par,&result));
    assert(result.sonic_action==9&&result.movement==SA3DS_SPRING_MOVE_AIR_CONTROL);
    assert(sa3ds_spring_action_select(10,16,1,par->nocon_speed,par,&result));
    assert(result.movement==SA3DS_SPRING_MOVE_UNCONTROLLED);
    assert(sa3ds_spring_action_select(10,16,1,nextafterf(par->nocon_speed,-INFINITY),par,&result));
    assert(result.movement==SA3DS_SPRING_MOVE_AIR_CONTROL);
    assert(sa3ds_spring_action_select(10,16,0,100,par,&result));
    assert(result.sonic_action==12&&result.animation_request==17&&result.movement==SA3DS_SPRING_MOVE_FALL);
    assert(sa3ds_spring_action_select(9,39,-1,100,par,&result));assert(result.animation_request==40);
    assert(sa3ds_spring_action_select(9,41,-1,100,par,&result));assert(result.animation_request==42);
    assert(!sa3ds_spring_action_select(10,16,NAN,0,par,&result));

    SA1PC2004LocalVector add;
    SA1PC2004MotionContext ctx={1,0,0,1,1};
    assert(sa3ds_spring_uncontrolled_add_velocity((SA1PC2004LocalVector){0,8,0},
        (SA1PC2004LocalVector){0,-par->weight,0},&ctx,par,&add));
    assert(add.forward==0&&add.side==0&&fabsf(add.normal+.16f)<1e-6f);
    assert(sa3ds_spring_uncontrolled_add_velocity((SA1PC2004LocalVector){3,8,2},
        (SA1PC2004LocalVector){0,-par->weight,0},&ctx,par,&add));
    assert(fabsf(add.forward+.008f)<1e-6f&&fabsf(add.side+.704f)<1e-6f);
    ctx.grounded_status=0;
    assert(sa3ds_spring_uncontrolled_add_velocity((SA1PC2004LocalVector){3,8,2},
        (SA1PC2004LocalVector){0,-par->weight,0},&ctx,par,&add));
    assert(fabsf(add.forward+.024f)<1e-6f&&fabsf(add.side+.8f)<1e-6f);
    ctx.grounded_status=1;
    assert(sa3ds_spring_uncontrolled_add_velocity((SA1PC2004LocalVector){.01f,0,-.01f},
        (SA1PC2004LocalVector){-10,-par->weight,10},&ctx,par,&add));
    assert(add.forward==-.01f&&add.side==.01f);
    ctx.status_0x4000=1;
    assert(!sa3ds_spring_uncontrolled_add_velocity((SA1PC2004LocalVector){0},
        (SA1PC2004LocalVector){0},&ctx,par,&add));

    SA3DSMovementPlacement p=placement(2);
    assert(sa3ds_springb_primary_overlap(&p,(SA1Vec3){17.999f,0,0},4));
    assert(!sa3ds_springb_primary_overlap(&p,(SA1Vec3){18,0,0},4));
    assert(!sa3ds_springb_primary_overlap(&p,(SA1Vec3){0,18,0},4));
    SA3DSSpringPrimaryCollider c={{0,0,0},{0,5.4f,0},4,4,.65f,3};
    p.type=1;
    assert(sa3ds_spring_primary_overlap(&p,&c)==1);
    c.task_origin.x=10;assert(sa3ds_spring_primary_overlap(&p,&c)==0);
    c.task_origin=(SA1Vec3){0,10,0};assert(sa3ds_spring_primary_overlap(&p,&c)==0);
    c.task_origin=(SA1Vec3){6,9,0};assert(sa3ds_spring_primary_overlap(&p,&c)==1);
    c.task_origin.x=7;assert(sa3ds_spring_primary_overlap(&p,&c)==0); /* Top inset uses .65+6. */
    c.task_origin=(SA1Vec3){0,-18,0};assert(sa3ds_spring_primary_overlap(&p,&c)==0);
    c.task_origin.y=-17.9f;assert(sa3ds_spring_primary_overlap(&p,&c)==1);
    c.gravity_axis=4;assert(sa3ds_spring_primary_overlap(&p,&c)==-1);
    p.type=2;assert(sa3ds_spring_primary_overlap(&p,&c)==1);
    SA3DSSpringHitEntry hits[16]={{0,0,1,0,0},{0,1,0,0,0},
        {0,1,1,0,4},{0,1,1,9,0},{0,1,1,1,0},{-1,1,1,0,0}};
    SA3DSSpringHitResult hit={0};
    assert(sa3ds_spring_query_player_hit(hits,16,0,&hit));
    assert(hit.slot==3&&hit.player_index==9); /* No skip to valid player1. */
    assert(sa3ds_spring_query_player_hit(hits,16,4,&hit));
    assert(hit.slot==4&&hit.player_index==1);
    assert(!sa3ds_spring_query_player_hit(hits,16,5,&hit));
    assert(!sa3ds_spring_query_player_hit(hits,16,16,&hit));
    assert(!sa3ds_spring_query_player_hit(hits,17,0,&hit));
    puts("Spring runtime: PASS (source task phases/cooldown, commands/status, timer, action9/10 dispatch, primary CCL predicates/list query; no native integration)");
    return 0;
}
