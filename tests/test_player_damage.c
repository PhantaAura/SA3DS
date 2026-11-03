#include "player_damage.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {unsigned calls;uint16_t value;} RNG;
static uint16_t random15(void*p){RNG*r=p;r->calls++;return r->value;}
static SA3DSPlayerDamageState initial(void){return (SA3DSPlayerDamageState){.action=8,.status=0x504,.rings=7,.animation=14,
    .position={1,2,3},.collision_up={0,1,0},.speed={4,5,6}};}
int main(void){
    RNG rng={.value=0};SA3DSPlayerDamageWorld world={.stage_id=1,.death_command_allowed=1,.random15=random15,.context=&rng};
    const SA1SteamPlayerParameters*p=&SA1_STEAM_SONIC_PARAMETER_EVIDENCE;
    SA3DSPlayerDamageState s=initial();SA3DSPlayerDamageResult r;
    assert(sa3ds_player_damage_tick(&s,&world,p,&r));
    assert(r.accepted&&s.action==16&&s.animation==23&&s.timer==120&&s.status==0x8000);
    assert(s.speed.forward==-p->run_speed&&s.speed.normal==p->jmp_y_spd&&s.speed.side==6);
    assert(r.scatter_count==7&&r.reset_rings&&s.rings==0&&rng.calls==1);
    assert(r.sound_count==2&&r.sound_ids[0]==0x4d1&&r.sound_ids[1]==0&&r.shader_index==8);
    assert(r.scatter_angles[3]==(int32_t)(154.*65536./360.));
    for(unsigned tick=0;tick<120;tick++){s.status|=4;assert(sa3ds_player_damage_tick(&s,&world,p,&r));assert(!r.accepted&&!(s.status&4)&&!r.sound_count);}
    assert(s.timer==0&&rng.calls==1);s.status|=4;s.rings=1;
    assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&r.accepted&&rng.calls==2&&r.scatter_count==1);
    s=initial();s.flags6=0x8000;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&!r.accepted&&s.timer==0&&(s.status&4)&&(s.status&0x8000));
    s=initial();s.flags6=3;unsigned calls=rng.calls;
    assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&r.accepted&&r.shield_consumed&&s.flags6==0&&s.rings==7&&rng.calls==calls&&r.sound_count==1);
    s=initial();s.flags4=0x8000;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&!r.accepted&&s.speed.forward==2&&s.timer==120);
    s=initial();s.action=45;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&!r.accepted&&s.timer==120&&!r.sound_count);
    s=initial();s.task_byte2=2;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&!r.accepted&&s.timer==0);
    s=initial();s.status=0;s.flags5c=0x10000;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&r.accepted);
    s=initial();s.rings=99;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&r.scatter_count==20&&s.rings==0);
    s=initial();s.rings=-1;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&!r.scatter_count&&s.rings==0&&!r.death_requested);
    s=initial();s.rings=0;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&r.death_requested&&s.next_command==50&&(s.status&0x1000)&&(s.flags6&0x4000)&&r.sound_ids[1]==0x17);
    s=initial();s.rings=0;world.game_mode=6;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&r.sound_ids[1]==0x597);
    world.death_command_allowed=0;s=initial();s.rings=0;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&!r.death_requested);
    s=initial();s.player_index=1;calls=rng.calls;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&r.accepted&&!r.reset_rings&&!r.scatter_count&&rng.calls==calls+1);
    s=initial();s.action=62;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&!r.accepted&&s.action==68&&s.animation==110&&!(s.status&4));
    s=initial();s.action=46;assert(sa3ds_player_damage_tick(&s,&world,p,&r)&&r.accepted&&r.water_effect&&s.action==50&&s.animation==46&&s.position.y==7&&s.angles[2]==0xc000);
    assert(s.speed.forward==.8f&&s.speed.side==6);
    SA3DSPlayerHurtResult h;s=initial();s.action=16;s.status=0x8001;s.animation=23;s.timer=120;
    assert(sa3ds_player_hurt_select(&s,0,0,&h)&&!h.write_animation);
    s.timer=119;assert(sa3ds_player_hurt_select(&s,0,0,&h)&&h.write_animation&&s.animation==24);
    s.timer=61;assert(sa3ds_player_hurt_select(&s,1,1,&h)&&!h.resume);
    s.timer=60;assert(sa3ds_player_hurt_select(&s,1,1,&h)&&h.begin_jump&&h.resume);
    s.status=0x8000;s.timer=0;assert(sa3ds_player_hurt_select(&s,0,0,&h)&&s.action==12&&s.animation==18);
    s.action=16;s.status=0x8001;s.animation=24;s.timer=0;assert(sa3ds_player_hurt_select(&s,0,0,&h)&&s.action==2&&h.ground_selector);
    s.action=16;s.animation=0;s.speed.forward=0;assert(sa3ds_player_hurt_select(&s,0,0,&h)&&s.action==1&&h.idle_selector);
    assert(sa3ds_player_damage_visible(120)&&!sa3ds_player_damage_visible(119)&&!sa3ds_player_damage_visible(118)&&sa3ds_player_damage_visible(117));
    puts("Player damage: PASS source preamble/immunity/branches, ordered effects, shield/ring/death requests, signed count/integer burst angles, hurt selector thresholds and blink; executors tested separately");
}
