/* Synthetic owner-free ACTION pose safety tests. Never embeds Sega data. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/game/sonic_action_preflight.h"

static void wr16(uint8_t *p, uint16_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void wr32(uint8_t *p, uint32_t v) {
    for (int i=0;i<4;i++) p[i]=(uint8_t)(v>>(8*i));
}
static void wrf(uint8_t *p,float v) { uint32_t u; memcpy(&u,&v,4); wr32(p,u); }

typedef struct {
    uint8_t bytes[512];
    SA3CCharacter c;
} Fixture;

static void init(Fixture *f) {
    memset(f,0,sizeof(*f));
    f->c.data=f->bytes; f->c.size=sizeof(f->bytes);
    f->c.sections[0]=(SA3CSection){{'N','O','D','E'},1,0,48,48};
    f->c.sections[7]=(SA3CSection){{'A','N','I','M'},1,64,24,24};
    f->c.sections[8]=(SA3CSection){{'C','H','A','N'},1,96,24,24};
    f->c.sections[9]=(SA3CSection){{'K','E','Y','S'},2,128,16,32};
    /* no model, child or sibling in the synthetic single-node tree */
    wr16(f->bytes,0xFFFFu); wr16(f->bytes+2,0xFFFFu); wr16(f->bytes+4,0xFFFFu);
    /* bind pose: position/rotation zero, unit scale */
    wrf(f->bytes+36,1.0f); wrf(f->bytes+40,1.0f); wrf(f->bytes+44,1.0f);
    /* action 0, root 0, 4 frames, one position channel */
    wr16(f->bytes+64+2,0); wr16(f->bytes+64+4,4);
    wr32(f->bytes+64+8,0); wr32(f->bytes+64+12,1);
    wr16(f->bytes+96,0); wr16(f->bytes+96+2,1);
    wr16(f->bytes+96+8,2);
    wr32(f->bytes+128,0); wrf(f->bytes+128+4,0.0f);
    wr32(f->bytes+144,3); wrf(f->bytes+144+4,6.0f);
}

static SA3DSActionPreflightResult run(Fixture *f, uint16_t slot,
                                      SA3DSActionPreflightReport *r) {
    SA3DSSonicAnimationBinding b={
        .runtime_id=9, .action_slot=slot, .source_root_node=0,
        .source_frame_count=(uint16_t)(f->bytes[68]|((uint16_t)f->bytes[69]<<8)),
        .name="synthetic"
    };
    return sa3ds_action_preflight(&f->c,&b,1,r);
}

int main(void) {
    Fixture f; SA3DSActionPreflightReport r;
    init(&f);
    assert(run(&f,0,&r)==SA3DS_ACTION_PREFLIGHT_OK);
    assert(r.checked_bindings==1 && r.checked_samples==3);
    assert(run(&f,3,&r)==SA3DS_ACTION_PREFLIGHT_SLOT);
    assert(r.failing_runtime_id==9 && r.failing_action_slot==3);
    init(&f); wr16(f.bytes+64+2,0xFFFFu);
    assert(run(&f,0,&r)==SA3DS_ACTION_PREFLIGHT_EMPTY);
    init(&f); wr16(f.bytes+64+6,0x0040u);
    assert(run(&f,0,&r)==SA3DS_ACTION_PREFLIGHT_POSE);
    assert(r.pose_error==SA3C_POSE_ERR_UNSUPPORTED_INTERPOLATION);
    init(&f); wrf(f.bytes+144+4,NAN);
    assert(run(&f,0,&r)==SA3DS_ACTION_PREFLIGHT_NONFINITE);
    init(&f);
    SA3CTransform local; uint16_t root;
    assert(sa3c_pose_sample_action(&f.c,0,NAN,0,&local,1,&root)==SA3C_POSE_ERR_ARGUMENT);
    assert(sa3c_pose_sample_action(&f.c,0,INFINITY,0,&local,1,&root)==SA3C_POSE_ERR_ARGUMENT);
    assert(sa3c_pose_sample_action(&f.c,0,-INFINITY,1,&local,1,&root)==SA3C_POSE_ERR_ARGUMENT);
    /* A hidden corrupt key can evade the old three-pose probe: the sampler
     * never touches the key at frame 1 when queried at frames 0, 5, and 10. */
    init(&f);
    f.c.sections[9].count=5; f.c.sections[9].size=80;
    wr16(f.bytes+64+4,11); wr16(f.bytes+96+8,5);
    const uint32_t key_frames[5]={0,1,2,3,10};
    for (unsigned i=0;i<5;i++) {
        wr32(f.bytes+128+i*16,key_frames[i]);
        wrf(f.bytes+128+i*16+4,(float)key_frames[i]);
    }
    wrf(f.bytes+128+16+4,NAN); /* hidden key at frame 1 */
    SA3CTransform sampled;
    for (unsigned i=0;i<3;i++) {
        float frame=(float)(i*5);
        assert(sa3c_pose_sample_action(&f.c,0,frame,0,&sampled,1,&root)==SA3C_POSE_OK);
        assert(isfinite(sampled.position.x)); /* old 3-sample test misses it */
    }
    assert(run(&f,0,&r)==SA3DS_ACTION_PREFLIGHT_NONFINITE);
    assert(r.failing_channel_index==0 && r.failing_track==1 && r.failing_key_index==1);
    assert(r.failing_frame==1.0f);
    /* A descending key violates the linear interpolation sampler's ordering. */
    init(&f); wr32(f.bytes+144,0);
    assert(run(&f,0,&r)==SA3DS_ACTION_PREFLIGHT_OK); /* equal keys accepted */
    init(&f); wr32(f.bytes+128,3); wr32(f.bytes+144,0);
    assert(run(&f,0,&r)==SA3DS_ACTION_PREFLIGHT_KEY_ORDER);
    assert(r.failing_key_index==1 && r.failing_track==1);
    /* An enabled track with no keys must not silently zero the bind pose. */
    init(&f); wr16(f.bytes+96+8,0);
    assert(run(&f,0,&r)==SA3DS_ACTION_PREFLIGHT_TRACK_EMPTY);
    assert(r.failing_channel_index==0 && r.failing_key_index==UINT32_MAX);
    init(&f); wr32(f.bytes+96+4,3);
    assert(run(&f,0,&r)==SA3DS_ACTION_PREFLIGHT_TRACK_RANGE);
    assert(sa3ds_action_preflight(NULL,NULL,0,&r)==SA3DS_ACTION_PREFLIGHT_ARGUMENT);
    puts("Sonic ACTION preflight: PASS (all-key integrity, hidden NaN, order, empty/range, pose safety)");
    return 0;
}
