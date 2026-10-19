#include "sa3c_gpu_batch.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void wr16(uint8_t *p, uint16_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
}
static void wr32(uint8_t *p, uint32_t v) {
    for (unsigned i=0;i<4;i++) p[i]=(uint8_t)(v>>(8u*i));
}

int main(void) {
    uint8_t data[256]={0};
    SA3CCharacter c={0};
    c.data=data; c.size=sizeof(data);
    c.sections[0]=(SA3CSection){{'N','O','D','E'},3,0,48,144};
    c.sections[1]=(SA3CSection){{'M','O','D','L'},2,144,32,64};
    c.sections[2]=(SA3CSection){{'M','E','S','H'},2,208,24,48};
    wr16(data+0,0); wr16(data+48,0); wr16(data+96,1);
    wr32(data+144,0); wr32(data+148,1);
    wr32(data+176,1); wr32(data+180,1);
    wr32(data+208+8,3); wr32(data+232+8,6);

    SA3DSGpuBatch batch;
    assert(sa3ds_gpu_batch_budget(&c,&batch)==SA3DS_BATCH_OK);
    /* Mesh0 is referenced by TWO nodes, so budget is 3+3+6, not 3+6. */
    assert(batch.vertex_capacity==12 && batch.draw_capacity==3);
    uint32_t a=UINT32_MAX,b=UINT32_MAX,d=UINT32_MAX;
    assert(sa3ds_gpu_batch_reserve(&batch,3,&a)==SA3DS_BATCH_OK && a==0);
    assert(sa3ds_gpu_batch_reserve(&batch,3,&b)==SA3DS_BATCH_OK && b==3);
    assert(sa3ds_gpu_batch_reserve(&batch,6,&d)==SA3DS_BATCH_OK && d==6);
    assert(a+3<=b && b+3<=d); /* no draw spans alias */
    assert(sa3ds_gpu_batch_reserve(&batch,1,&a)==SA3DS_BATCH_CAPACITY);
    sa3ds_gpu_batch_reset(&batch);
    assert(batch.cursor==0 && batch.draws_used==0);
    assert(sa3ds_gpu_batch_reserve(&batch,6,&a)==SA3DS_BATCH_OK && a==0);
    assert(sa3ds_gpu_batch_reserve(&batch,7,&b)==SA3DS_BATCH_CAPACITY);
    assert(sa3ds_gpu_batch_reserve(&batch,0,&b)==SA3DS_BATCH_ARGUMENT);
    assert(sa3ds_gpu_batch_reserve(NULL,1,&b)==SA3DS_BATCH_ARGUMENT);

    wr16(data+96,2); /* bad node->model reference */
    assert(sa3ds_gpu_batch_budget(&c,&batch)==SA3DS_BATCH_BAD_REFERENCE);
    wr16(data+96,1);
    wr32(data+232+8,UINT32_MAX); /* impossible Old 3DS reservation */
    assert(sa3ds_gpu_batch_budget(&c,&batch)==SA3DS_BATCH_OVERFLOW);
    wr32(data+232+8,6);
    wr16(data+0,0xFFFF); wr16(data+48,0xFFFF); wr16(data+96,0xFFFF);
    assert(sa3ds_gpu_batch_budget(&c,&batch)==SA3DS_BATCH_EMPTY);
    c.sections[2].size=10000;
    assert(sa3ds_gpu_batch_budget(&c,&batch)==SA3DS_BATCH_ARGUMENT);
    assert(strcmp(sa3ds_gpu_batch_result_name(SA3DS_BATCH_CAPACITY),
                  "FRAME_VERTEX_ARENA_EXHAUSTED")==0);
    puts("SA3DS GPU frame vertex arena: PASS (per-draw isolation, duplicate instances, "
         "reset, capacity, bad refs, overflow, empty, truncation)");
    return 0;
}
