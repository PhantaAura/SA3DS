#include "owner_assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SA3DSAssetReadResult sa3ds_read_asset_stream(FILE *stream, size_t limit,
                                            uint8_t **out_data, size_t *out_size) {
    if (!out_data || !out_size) {
        if (stream) fclose(stream);
        return SA3DS_ASSET_READ_IO;
    }
    *out_data = NULL;
    *out_size = 0;
    if (!stream) return SA3DS_ASSET_READ_IO;
    long length = 0;
    if (fseek(stream, 0, SEEK_END) || (length = ftell(stream)) <= 0 ||
        (uintmax_t)length > limit || fseek(stream, 0, SEEK_SET)) {
        fclose(stream);
        return SA3DS_ASSET_READ_SIZE;
    }
    size_t size = (size_t)length;
    uint8_t *data = malloc(size);
    if (!data) {
        fclose(stream);
        return SA3DS_ASSET_READ_MEMORY;
    }
    /* A file can grow between the size check and the read. */
    int complete = fread(data, 1, size, stream) == size &&
                   fgetc(stream) == EOF && feof(stream) && !ferror(stream);
    if (fclose(stream) != 0) complete = 0;
    if (!complete) {
        free(data);
        return SA3DS_ASSET_READ_IO;
    }
    *out_data = data;
    *out_size = size;
    return SA3DS_ASSET_READ_OK;
}

int sa3ds_read_file(const char *path, uint8_t **out_data, size_t *out_size) {
    if (!out_data || !out_size) return 0;
    FILE *stream = path ? fopen(path, "rb") : NULL;
    return sa3ds_read_asset_stream(stream, SA3DS_OWNER_ASSET_MAX_BYTES,
                                  out_data, out_size) == SA3DS_ASSET_READ_OK;
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
