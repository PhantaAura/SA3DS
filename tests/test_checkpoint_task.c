#include "checkpoint_task.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
    SA3DSCheckpointTask t={0};SA3DSCheckpointInput in={.player_present=1};SA3DSCheckpointResult out;
    assert(sa3ds_checkpoint_task_tick(&t,&in,&out));
    assert(out.initialize&&!out.display&&!out.enqueue_collision&&t.action==1&&t.post_x==16384&&t.post_y==-16384);
    in.prior_collision=1;in.other_is_player0=0;
    assert(sa3ds_checkpoint_task_tick(&t,&in,&out)&&t.action==1&&!out.capture_restart);
    int16_t spin;assert(sa3ds_checkpoint_spin(2.5,&spin)&&spin==4551);
    in.other_is_player0=1;in.activation_spin=spin;
    assert(sa3ds_checkpoint_task_tick(&t,&in,&out));
    assert(t.action==2&&t.time_display==300&&t.velocity_y==4551);
    assert(out.display&&!out.active_material&&out.draw_post_x==16384&&out.child_post_y==-16384);
    assert(out.capture_restart&&out.persist_set&&out.sound_id==10&&!out.display_time&&!out.emit_effect);
    in.prior_collision=0;int effects=0,shown=0;unsigned ticks=0;
    do{
        int32_t old_x=t.post_x,old_y=t.post_y;
        assert(sa3ds_checkpoint_task_tick(&t,&in,&out));
        assert(out.display&&out.active_material&&out.draw_post_x==old_x&&out.child_post_y==old_y);
        effects+=out.emit_effect;shown+=out.display_time;ticks++;
        assert(!out.capture_restart&&!out.persist_set&&!out.play_sound);
        if(ticks==1)assert(t.post_y==-11833&&t.velocity_y==4528&&t.time_display==299);
        assert(ticks<10000);
    }while(t.action==2||t.time_display);
    assert(effects==3&&shown==240&&t.action==3&&t.velocity_x==0&&t.velocity_y==0);
    assert(sa3ds_checkpoint_spin(-2.5,&spin)&&spin==-4551);
    assert(!sa3ds_checkpoint_spin(NAN,&spin));
    SA3DSCheckpointTask saved={0};in.persisted=1;
    assert(sa3ds_checkpoint_task_tick(&saved,&in,&out)&&saved.action==3&&saved.post_x==0&&saved.post_y==-16384);
    assert(!out.capture_restart&&!out.persist_set&&!out.play_sound&&!out.display);
    in.clipped=1;saved.action=1;
    assert(sa3ds_checkpoint_task_tick(&saved,&in,&out)&&saved.action==1&&!out.display&&!out.enqueue_collision);
    in.clipped=0;in.persisted=0;in.player_present=0;
    t=(SA3DSCheckpointTask){.action=2,.post_x=123,.post_y=456,.velocity_y=1000,.time_display=300};
    assert(sa3ds_checkpoint_task_tick(&t,&in,&out)&&t.post_x==123&&t.post_y==456&&t.velocity_y==1000&&t.time_display==299&&!out.emit_effect);
    /*All source int16 impulses: exact 0.995f multiply/truncate, including sign.*/
    in.player_present=1;
    for(int n=-32768;n<=32767;n++){
        t=(SA3DSCheckpointTask){.action=2,.velocity_y=n,.effect_count=3};
        assert(sa3ds_checkpoint_task_tick(&t,&in,&out));
        assert(t.velocity_y==(int32_t)trunc((double)n*(double)0.995f));
    }
    /*Spring-force cutoff and signed-word wrapping against independent x87-
     * style floating arithmetic, rather than the implementation's ratios.*/
    const int positions[]={-32768,-1,0,8192,8193,16384,32767};
    const int velocities[]={-32768,-3659,-3640,-1,0,1,3640,3659,32767};
    for(unsigned x=0;x<sizeof(positions)/sizeof(*positions);x++)for(unsigned v=0;v<sizeof(velocities)/sizeof(*velocities);v++){
        int final_x=positions[x];
        int vy=(int)truncl((long double)velocities[v]*(long double)0.995f),vx=0;
        int magnitude=vy<0?-vy:vy;
        if(magnitude<3640){
            int target=final_x>8192?18386-final_x:final_x;
            target=(int)((unsigned)target&65535u);if(target>=32768)target-=65536;
            vx=(int)truncl((long double)(3640-magnitude)*target*(long double)-1.220703143189894e-5f);
            vx=(int)((unsigned)vx&65535u);if(vx>=32768)vx-=65536;
        }
        t=(SA3DSCheckpointTask){.action=2,.post_x=final_x,.velocity_y=velocities[v],.effect_count=3};
        assert(sa3ds_checkpoint_task_tick(&t,&in,&out)&&t.velocity_x==vx&&t.velocity_y==vy);
    }
    t=(SA3DSCheckpointTask){.action=4};assert(sa3ds_checkpoint_task_tick(&t,&in,&out));
    assert(out.display&&out.active_material&&!out.enqueue_collision&&!out.sync_children);
    printf("Checkpoint task: PASS init/persist/CCL identity, display-before-activation ordering, post spin/settle (%u ticks),3 effects,300-count blink and all65536 signed damping values; native task integration separate\n",ticks);
}
