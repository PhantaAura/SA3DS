#ifndef SA3DS_0E_OWNER_ASSETS_H
#define SA3DS_0E_OWNER_ASSETS_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Conservative per-file cap for the Old 3DS address space. A larger owner
 * package must be investigated, not silently truncated or over-allocated. */
#define SA3DS_OWNER_ASSET_MAX_BYTES (16u * 1024u * 1024u)

typedef enum {
    SA3DS_OWNER_SOURCE_NONE = 0,
    SA3DS_OWNER_SOURCE_ROMFS = 1,
    SA3DS_OWNER_SOURCE_SD = 2
} SA3DSOwnerSource;

typedef enum {
    SA3DS_ASSET_READ_OK,
    SA3DS_ASSET_READ_SIZE,
    SA3DS_ASSET_READ_MEMORY,
    SA3DS_ASSET_READ_IO
} SA3DSAssetReadResult;

/* Takes ownership of stream. Publishes bytes only after an exact read and close. */
SA3DSAssetReadResult sa3ds_read_asset_stream(FILE *stream, size_t limit,
                                            uint8_t **out_data, size_t *out_size);
int sa3ds_read_file(const char *path, uint8_t **out_data, size_t *out_size);
/* Load an entire SA3C/SA3T pair from ONE location. Never mix a RomFS SA3C
 * with an SD SA3T. Paths are directory prefixes, e.g. romfs:/sa3ds and
 * sdmc:/3ds/SA3DS/assets. Fallback only after primary pair fails. */
int sa3ds_load_owner_pair(const char *romfs_dir, const char *sd_dir,
                          uint8_t **out_character, size_t *out_character_size,
                          uint8_t **out_textures, size_t *out_textures_size,
                          SA3DSOwnerSource *out_source);
const char *sa3ds_owner_source_name(SA3DSOwnerSource source);
void sa3ds_free_file(void *data);

#endif
