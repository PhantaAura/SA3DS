#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/game/sonic_animation_runtime.h"
#include "../src/game/sonic_animation_sync.h"
#include "../src/sa1/physics/steam_player_parameters.h"

static uint32_t lcg(uint32_t *s) { *s=*s*1664525u+1013904223u; return *s; }
static float sf(uint32_t *s, float scale) { return ((float)(lcg(s)&0xffffu)/32767.5f-1.0f)*scale; }

int main(void) {
    SA3DSSonicAnimationBinding b;
    assert(sa3ds_sonic_animation_binding(32,&b) && b.action_slot==21);
    assert(sa3ds_sonic_animation_binding(15,&b) && b.action_slot==14);
    assert(sa3ds_sonic_animation_binding(14,&b) && b.action_slot==14);
    assert(sa3ds_sonic_animation_binding(19,&b) && b.action_slot==11);

    uint32_t seed=0x13579bdu;
    for (unsigned i=0;i<1024;i++) {
        SA1PlayerState p;
        memset(&p,0,sizeof(p));
        p.pos=(SA1Vec3){sf(&seed,20),sf(&seed,10),sf(&seed,200)};
        p.vel=(SA1Vec3){sf(&seed,3),sf(&seed,3),sf(&seed,3)};
        p.yaw=sf(&seed,3.14159f);
        p.grounded=(lcg(&seed)&1u)!=0;
        p.surface_id=(int32_t)(lcg(&seed)%220u);
        p.support_distance=(float)(lcg(&seed)%100u)/50.0f;
        p.sim_frame=i;
        p.action_state=(SA1ActionState)(lcg(&seed)%4u);
        p.ball_form=(p.action_state==SA1_ACTION_ROLL || p.action_state==SA1_ACTION_SPINDASH_CHARGE || !p.grounded);
        p.spindash_charge=(p.action_state==SA1_ACTION_SPINDASH_CHARGE)?(float)(lcg(&seed)%100u)/20.0f:0.0f;
        SA1PlayerState before=p;
        SA3DSSonicAnimationSync sync={0};
        SA3DSSonicAnimationRuntime runtime={0};
        SA3DSSonicAnimationObservation o=sa3ds_sonic_animation_observe(&sync,&p,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
        assert(memcmp(&before,&p,sizeof(p))==0);
        SA3DSSonicAnimationSelection sel=sa3ds_sonic_animation_select(&runtime,&p,&o);
        assert(memcmp(&before,&p,sizeof(p))==0);
        assert(sel.selector_is_provisional==1);
    }
    puts("animation observer 1024-state non-mutation stress: PASS");
    return 0;
}
