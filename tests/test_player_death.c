#include "player_death_player.h"
#include "player_damage_player.h"
#include "stage_restart.h"
#include "sonic_animation_runtime.h"
#include "../src/sa1/physics/steam_flat.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static SA1Surface floor_surface(void*c,float x,float z){(void)c;(void)x;(void)z;return (SA1Surface){.ground_y=0,.normal={0,1,0},.valid=true};}
static uint16_t random15(void*c){(void)c;return 0;}
int main(void){
    SA3DSPlayerProtectionTask protection={.active=1};uint16_t flags=0x4003;uint8_t destroy;
    SA3DSPlayerDamageWorld protection_world={0};
    for(unsigned tick=1;tick<=121;tick++){
        assert(sa3ds_player_protection_tick(&protection,&flags,&destroy));
        assert(protection.counter==tick&&destroy==(tick==121));
        assert(flags==(tick<=120?0xc003:0x4003));
        SA3DSPlayerDamageState immune={.status=4,.flags6=(uint16_t)(flags&~0x4000u),.timer=99};
        SA3DSPlayerDamageResult result;
        assert(sa3ds_player_damage_tick(&immune,&protection_world,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE,&result));
        if(tick<=120)assert(!result.checked&&!result.accepted&&immune.timer==0&&(immune.status&0x8000));
        else assert(immune.timer==98); /*normal hurt-timer path restored*/
    }
    assert(sa3ds_player_protection_tick(&protection,&flags,&destroy)&&!destroy&&flags==0x4003);
    protection=(SA3DSPlayerProtectionTask){.counter=119,.active=1};
    assert(sa3ds_player_protection_tick(&protection,NULL,&destroy)&&!destroy);
    assert(sa3ds_player_protection_tick(&protection,NULL,&destroy)&&destroy);
    protection=(SA3DSPlayerProtectionTask){.counter=65535,.active=1};flags=3;
    assert(sa3ds_player_protection_tick(&protection,&flags,&destroy)&&!destroy&&protection.counter==0&&flags==0x8003);

    SA3DSPlayerDeathTask task={.active=1};SA3DSPlayerDeathTaskResult death;
    for(unsigned n=0;n<121;n++){assert(sa3ds_player_death_task_tick(&task,0,1,&death));assert(!death.stage_request&&!death.destroy);}
    assert(task.counter==121);assert(sa3ds_player_death_task_tick(&task,1,1,&death));
    assert(task.counter==122&&!task.active&&death.stage_request==2&&death.parent_status4&&death.audio_cleanup&&death.destroy);
    assert(sa3ds_player_death_task_tick(&task,0,1,&death)&&!death.stage_request);
    task=(SA3DSPlayerDeathTask){.active=1};assert(sa3ds_player_death_task_tick(&task,1,0,&death));assert(death.destroy&&!death.stage_request&&task.counter==1);
    task=(SA3DSPlayerDeathTask){.active=1,.counter=UINT32_MAX};assert(sa3ds_player_death_task_tick(&task,0,0,&death));assert(death.stage_request==2&&task.counter==0&&!death.parent_status4);
    SA3DSPlayerDamageState state={.status=0xffff,.next_command=50};uint16_t sound;uint8_t channel;
    assert(sa3ds_player_death_command(&state,0,&sound,&channel));assert(state.action==86&&state.animation==86&&state.status==0xdaff&&state.next_command==50&&sound==0x5de&&channel==0);
    assert(sa3ds_player_death_command(&state,1,&sound,&channel)&&sound==0x7fe&&channel==1);
    SA1PhysicsWorld world={.query_surface=floor_surface,.reset_position={0,1,0},.reset_min_x=-10000,.reset_max_x=10000,.reset_min_y=-10000,.reset_min_z=-10000,.reset_max_z=10000};
    SA1PlayerState p;sa1_player_reset(&p,&world);SA1Input input={.move_z=1,.jump_pressed=1,.jump_held=1};
    SA3DSPlayerDamageWorld damage_world={.stage_id=1,.game_mode=4,.death_command_allowed=1,.random15=random15};
    SA3DSSonicAnimationRuntime animation={0};SA3DSSonicAnimationSync sync={0};int16_t rings=0;
    unsigned clamps=0,air=0,ground=0;
    for(unsigned tick=0;tick<180;tick++){
        SA3DSPlayerDamageResult result;
        int ready=sa3ds_player_damage_prepare(&p,&input,tick==0?4:0,animation.initialized?animation.requested_runtime_id:0,&rings,&damage_world,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE,&result);
        if(!tick){assert(ready==2&&result.death_requested);assert(sa3ds_player_death_player_command(&p,0,&sound,&channel));}
        else assert(ready==1);
        sa1_physics_step_steam_sonic_flat(&p,&input,0,&world,&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,0);
        assert(p.pc2004_action_state==86&&p.action_state==SA1_ACTION_DEAD&&!p.ball_form);
        if(p.grounded)ground++;else air++;
        SA3DSSonicAnimationObservation observation=sa3ds_sonic_animation_observe(&sync,&p,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
        SA1PlayerState before=p;SA3DSSonicAnimationSelection selected=sa3ds_sonic_animation_select(&animation,&p,&observation);assert(!memcmp(&before,&p,sizeof(p)));
        assert(selected.requested_runtime_id==87&&selected.binding.action_slot==118&&selected.sample_frame<=69);
        if(selected.sample_frame==69)clamps++;
    }
    assert(air&&ground&&clamps&&p.pc2004_hurt_timer==0&&p.pc2004_action_state==86);
    SA3DSStageRestart flow;SA3DSStageRestartResult phase;sa3ds_stage_restart_init(&flow);assert(flow.lives==4);
    assert(sa3ds_stage_restart_begin(&flow,1,&phase)&&phase.run_world&&phase.process_requests&&phase.controls_enabled);
    flow.request=2;assert(sa3ds_stage_restart_end(&flow,&phase)&&flow.state==6);
    unsigned out=0,in=0,recover=0;
    do{assert(sa3ds_stage_restart_begin(&flow,1,&phase));out++;}while(!phase.clear_world&&out<100);
    assert(out==46&&flow.state==2&&flow.lives==3&&phase.clear_rings&&!phase.run_world&&flow.fade_alpha==255);
    do{assert(sa3ds_stage_restart_begin(&flow,1,&phase));in++;assert(!phase.run_world);}while(!phase.reload_stage&&in<100);
    assert(in==6&&flow.state==4&&flow.restart_count==1&&flow.fade_value==285);
    do{assert(sa3ds_stage_restart_begin(&flow,1,&phase));recover++;assert(phase.run_world&&!phase.process_requests&&!phase.controls_enabled);}while(flow.state!=15&&recover<100);
    assert(recover==49&&flow.fade_alpha==0);
    flow.request=2;assert(sa3ds_stage_restart_begin(&flow,1,&phase)&&!phase.run_world&&phase.process_requests&&!phase.controls_enabled);assert(sa3ds_stage_restart_end(&flow,&phase)&&flow.state==6);
    assert(sa3ds_stage_restart_begin(&flow,20,&phase));assert(sa3ds_stage_restart_begin(&flow,20,&phase)&&flow.fade_value==36);
    flow.lives=127;sa3ds_stage_restart_add_lives(&flow,1);assert(flow.lives==127);
    flow.lives=-2;sa3ds_stage_restart_add_lives(&flow,1);assert(flow.lives==127);
    flow.lives=0;sa3ds_stage_restart_add_lives(&flow,-1);assert(flow.lives==-1);
    flow.state=15;flow.request=2;assert(sa3ds_stage_restart_begin(&flow,1,&phase));assert(sa3ds_stage_restart_end(&flow,&phase)&&flow.state==8);
    for(unsigned n=1;n<=43;n++){
        assert(sa3ds_stage_restart_begin(&flow,1,&phase));
        assert(phase.run_world&&!phase.controls_enabled&&!phase.game_over_pending);
        assert(flow.fade_alpha==(n<43?n*6:255));
        assert(phase.clear_world==(n==43)&&phase.clear_rings==(n==43));
    }
    assert(flow.state==18&&flow.lives==-1);
    assert(sa3ds_stage_restart_begin(&flow,1,&phase)&&phase.game_over_pending&&!phase.run_world);
    sa3ds_stage_restart_init(&flow);flow.state=0;unsigned fresh=0;
    do{assert(sa3ds_stage_restart_begin(&flow,1,&phase));fresh++;assert(!phase.run_world);}while(!phase.reload_stage&&fresh<100);
    assert(fresh==6&&flow.state==4&&flow.restart_count==0&&flow.lives==4);
    fresh=0;do{assert(sa3ds_stage_restart_begin(&flow,1,&phase));fresh++;assert(phase.run_world&&!phase.controls_enabled);}while(flow.state!=15&&fresh<100);
    assert(fresh==49&&flow.fade_alpha==0&&flow.restart_count==0);
    printf("Death/restart: PASS command50/86,122nd-update task request/order/special/wrap,passive original118 pose (%u air/%u ground/%u clamp),fade46/6/49 boundaries,43-update game-over fade and signed lives; game-over executor separate\n",air,ground,clamps);
}
