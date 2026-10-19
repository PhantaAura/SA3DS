/* Portable, read-only preflight of user-generated Sonic SA3C packages.
 * Does not extract, write, or embed Sega-owned data. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../src/sa3c/sa3c_character.h"
#include "../src/game/sonic_animation_runtime.h"
#include "../src/game/sonic_action_preflight.h"

#define MAX_SA3C_BYTES (128u * 1024u * 1024u)

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s path/to/sonic.sa3c\n", argv[0]);
        return 2;
    }
    FILE *file = fopen(argv[1], "rb");
    if (!file) { perror("open SA3C"); return 2; }
    if (fseek(file, 0, SEEK_END) || ftell(file) < 0) {
        fprintf(stderr, "unable to measure SA3C file\n"); fclose(file); return 2;
    }
    long n = ftell(file);
    if (n < 272 || (unsigned long)n > MAX_SA3C_BYTES || fseek(file, 0, SEEK_SET)) {
        fprintf(stderr, "invalid or oversized SA3C file\n"); fclose(file); return 2;
    }
    uint8_t *data = (uint8_t *)malloc((size_t)n);
    if (!data) { fclose(file); fprintf(stderr, "out of memory\n"); return 2; }
    if (fread(data, 1, (size_t)n, file) != (size_t)n) {
        fprintf(stderr, "short SA3C read\n"); fclose(file); free(data); return 2;
    }
    fclose(file);
    SA3CCharacter c;
    SA3CResult rc = sa3c_character_open(&c, data, (size_t)n);
    if (rc == SA3C_OK) rc = sa3c_character_validate_references(&c);
    if (rc != SA3C_OK) {
        fprintf(stderr, "SA3C format/references: %s\n", sa3c_result_name(rc));
        free(data); return 1;
    }
    const size_t count = sa3ds_sonic_animation_binding_count();
    SA3DSSonicAnimationBinding *bindings = calloc(count, sizeof(*bindings));
    if (!bindings) { free(data); return 2; }
    for (size_t i = 0; i < count; ++i) {
        if (!sa3ds_sonic_animation_binding_at(i, &bindings[i])) {
            free(bindings); free(data); return 2;
        }
    }
    SA3DSActionPreflightReport report;
    SA3DSActionPreflightResult result = sa3ds_action_preflight(&c, bindings, count, &report);
    free(bindings); free(data);
    if (result != SA3DS_ACTION_PREFLIGHT_OK) {
        fprintf(stderr, "ACTION preflight: %s; runtime=%u slot=%u frame=%.2f "
                        "channel=%lu track=%u key=%lu pose=%s\n",
            sa3ds_action_preflight_result_name(result), report.failing_runtime_id,
            report.failing_action_slot, report.failing_frame,
            (unsigned long)report.failing_channel_index,
            (unsigned)report.failing_track, (unsigned long)report.failing_key_index,
            sa3c_pose_result_name(report.pose_error));
        return 1;
    }
    printf("SA3C ACTION preflight PASS: %lu bindings, %lu sampled poses; "
           "all active track keys scanned\n",
        (unsigned long)report.checked_bindings,
        (unsigned long)report.checked_samples);
    return 0;
}
