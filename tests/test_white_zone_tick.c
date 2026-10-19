#include "../src/game/white_zone_tick.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static SA1Input sample(void) {
    SA1Input x={0}; x.move_z=0.75f; x.jump_held=true; return x;
}
int main(void) {
    SA3DSWhiteZoneTick c;
    sa3ds_wz_tick_reset(&c);
    SA1Input in=sample();
    /* 1000 ms divided across irregular rendering must produce 60 ticks. */
    unsigned ticks=0;
    for (unsigned i=0;i<1000;i++) {
        unsigned n=sa3ds_wz_tick_begin(&c,1,&in);
        ticks+=n;
        for (unsigned j=0;j<n;j++) {
            SA1Input x=sa3ds_wz_tick_input(&c,j);
            assert(x.move_z==0.75f && x.jump_held);
        }
    }
    assert(ticks==60 && c.total_ticks==60 && c.phase==0 && c.dropped_ticks==0);
    sa3ds_wz_tick_reset(&c);
    ticks=0;
    for (unsigned i=0;i<60;i++) {
        unsigned n=sa3ds_wz_tick_begin(&c,(i%3==0)?17:((i%3==1)?16:17),&in);
        ticks+=n;
        for(unsigned j=0;j<n;j++) (void)sa3ds_wz_tick_input(&c,j);
    }
    assert(ticks==60 && c.phase==0);
    /* A press on a render frame without a sim tick must not be lost. */
    sa3ds_wz_tick_reset(&c);
    in.jump_pressed=true;
    assert(sa3ds_wz_tick_begin(&c,3,&in)==0);
    in.jump_pressed=false;
    assert(sa3ds_wz_tick_begin(&c,14,&in)==1);
    SA1Input x=sa3ds_wz_tick_input(&c,0);
    assert(x.jump_pressed && x.jump_held);
    assert(sa3ds_wz_tick_begin(&c,17,&in)==1);
    x=sa3ds_wz_tick_input(&c,0);
    assert(!x.jump_pressed && x.jump_held);
    /* Multiple simulation ticks on one frame consume the edge only once. */
    sa3ds_wz_tick_reset(&c);
    in.action_pressed=true;
    assert(sa3ds_wz_tick_begin(&c,50,&in)==3);
    assert(sa3ds_wz_tick_input(&c,0).action_pressed);
    assert(!sa3ds_wz_tick_input(&c,1).action_pressed);
    assert(!sa3ds_wz_tick_input(&c,2).action_pressed);
    /* Time spikes cannot induce an unbounded catch-up spiral. */
    sa3ds_wz_tick_reset(&c);
    in.action_pressed=false;
    assert(sa3ds_wz_tick_begin(&c,30000,&in)==SA3DS_WZ_MAX_STEPS_PER_FRAME);
    assert(c.dropped_ticks==11 && c.phase==0);
    for(unsigned i=0;i<4;i++) (void)sa3ds_wz_tick_input(&c,i);
    /* Reset discards queued edges, accumulated time and old replay ticks. */
    sa3ds_wz_tick_reset(&c);
    assert(c.total_ticks==0 && c.dropped_ticks==0 && c.phase==0);
    assert(!sa3ds_wz_tick_input(&c,0).action_pressed);
    /* Two identical sequences produce exactly identical clock state. */
    SA3DSWhiteZoneTick a,b;
    sa3ds_wz_tick_reset(&a);sa3ds_wz_tick_reset(&b);
    for(unsigned i=0;i<200;i++) {
        in.jump_pressed=(i%31==0);
        unsigned na=sa3ds_wz_tick_begin(&a,i%19,&in);
        unsigned nb=sa3ds_wz_tick_begin(&b,i%19,&in);
        assert(na==nb);
        for(unsigned j=0;j<na;j++) {
            SA1Input ia=sa3ds_wz_tick_input(&a,j);
            SA1Input ib=sa3ds_wz_tick_input(&b,j);
            assert(memcmp(&ia,&ib,sizeof(ia))==0);
        }
    }
    assert(a.phase==b.phase && a.total_ticks==b.total_ticks &&
           a.dropped_ticks==b.dropped_ticks);
    puts("White Zone 60Hz scheduler: PASS (render decoupling, edge latch, catchup, reset, replay)");
    return 0;
}
