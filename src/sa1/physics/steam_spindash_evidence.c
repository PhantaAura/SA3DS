#include "steam_spindash_evidence.h"

float sa1_sadx_spindash_initial_charge(float local_forward_speed) {
    return local_forward_speed >= SA1_SPINDASH_INITIAL_MIN_PC2004_EVIDENCE
        ? local_forward_speed
        : SA1_SPINDASH_INITIAL_MIN_PC2004_EVIDENCE;
}

float sa1_sadx_spindash_charge_held_tick(float charge) {
    if (charge < SA1_SPINDASH_MAX_COMPARE_PC2004_EVIDENCE)
        charge += SA1_SPINDASH_INCREMENT_PC2004_EVIDENCE;
    return charge;
}

int sa1_sadx_spindash_timeout_reached(uint32_t spin_timer) {
    return spin_timer >= SA1_SPINDASH_TIMEOUT_FRAMES_PC2004_EVIDENCE;
}
