#include "owner_assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sa3ds_read_file(const char *path, uint8_t **out_data, size_t *out_size) {
    if (!out_data || !out_size) return 0;
    *out_data = NULL;
    *out_size = 0;
    if (!path) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long n = ftell(f);
    if (n <= 0 || (unsigned long)n > SA3DS_OWNER_ASSET_MAX_BYTES) {
        fclose(f); return 0;
    }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
    uint8_t *p = (uint8_t*)malloc((size_t)n);
    if (!p) { fclose(f); return 0; }
    size_t got = fread(p, 1, (size_t)n, f);
    int bad = (got != (size_t)n) || ferror(f);
    if (fclose(f) != 0) bad = 1;
    if (bad) { free(p); return 0; }
    *out_data = p;
    *out_size = (size_t)n;
    return 1;
}

static int try_pair(const char *dir, uint8_t **c, size_t *cs,
                    uint8_t **t, size_t *ts) {
    if (!dir || !*dir) return 0;
    char cp[256], tp[256];
    int cn = snprintf(cp, sizeof(cp), "%s/sonic.sa3c", dir);
    int tn = snprintf(tp, sizeof(tp), "%s/sonic.sa3t", dir);
    if (cn < 0 || tn < 0 || (size_t)cn >= sizeof(cp) ||
        (size_t)tn >= sizeof(tp)) return 0;
    if (!sa3ds_read_file(cp, c, cs)) return 0;
    if (!sa3ds_read_file(tp, t, ts)) {
        free(*c); *c = NULL; *cs = 0;
        return 0;
    }
    return 1;
}

int sa3ds_load_owner_pair(const char *romfs_dir, const char *sd_dir,
                          uint8_t **out_character, size_t *out_character_size,
                          uint8_t **out_textures, size_t *out_textures_size,
                          SA3DSOwnerSource *out_source) {
    if (!out_character || !out_character_size || !out_textures ||
        !out_textures_size || !out_source) return 0;
    *out_character = NULL; *out_character_size = 0;
    *out_textures = NULL; *out_textures_size = 0;
    *out_source = SA3DS_OWNER_SOURCE_NONE;
    if (try_pair(romfs_dir, out_character, out_character_size,
                 out_textures, out_textures_size)) {
        *out_source = SA3DS_OWNER_SOURCE_ROMFS;
        return 1;
    }
    if (try_pair(sd_dir, out_character, out_character_size,
                 out_textures, out_textures_size)) {
        *out_source = SA3DS_OWNER_SOURCE_SD;
        return 1;
    }
    return 0;
}

const char *sa3ds_owner_source_name(SA3DSOwnerSource source) {
    switch (source) {
        case SA3DS_OWNER_SOURCE_ROMFS: return "ROMFS";
        case SA3DS_OWNER_SOURCE_SD: return "SD";
        default: return "NONE";
    }
}

void sa3ds_free_file(void *data) { free(data); }
