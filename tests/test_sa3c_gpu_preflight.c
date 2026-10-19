#include "sa3c_character.h"
#include "sa3c_gpu_preflight.h"
#include "sa3t_texture.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *readall(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n <= 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    uint8_t *b = (uint8_t *)malloc((size_t)n);
    if (!b || fread(b, 1, (size_t)n, f) != (size_t)n) {
        free(b); fclose(f); return NULL;
    }
    fclose(f); *size = (size_t)n; return b;
}
static void wr16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
static void wr32(uint8_t *p, uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (i * 8));
}

int main(int argc, char **argv) {
    assert(sa3ds_visual_material_rgba(0xFF000000u) == 0x000000FFu);
    assert(sa3ds_visual_material_rgba(0x80336699u) == 0x33669980u);
    assert(sa3ds_visual_material_rgba(0x00FFFFFFu) == 0xFFFFFF00u);
    assert(sa3ds_visual_material_rgba(0xFFB2B2B2u) == 0xB2B2B2FFu);
    if (argc != 3) {
        fprintf(stderr, "usage: %s OWNER_GENERATED.sa3c OWNER_GENERATED.sa3t\n", argv[0]);
        return 2;
    }
    size_t cs = 0, ts = 0;
    uint8_t *cb = readall(argv[1], &cs), *tb = readall(argv[2], &ts);
    assert(cb && tb);
    SA3CCharacter c;
    SA3TArchive t;
    assert(sa3c_character_open(&c, cb, cs) == SA3C_OK);
    assert(sa3c_character_validate_references(&c) == SA3C_OK);
    assert(sa3t_open(&t, tb, ts) == SA3T_OK);
    SA3DSVisualReport report;
    assert(sa3ds_visual_preflight(&c, &t, &report) == SA3DS_VISUAL_OK);
    assert(report.textures == 16 && report.materials > 0);
    assert(report.referenced_textures == 13);
    assert(report.rgba8_bytes == 232192u);
    assert(report.nonopaque_materials == 0);
    assert(report.fully_transparent_texels + report.partially_transparent_texels <=
           report.rgba8_bytes / 4);

    /* Corrupt one metadata dimension: package parsing still succeeds, but
     * the paired SA3C/SA3T GPU upload MUST fail before allocating textures. */
    const size_t texmeta = c.sections[6].offset;
    uint16_t w = (uint16_t)(cb[texmeta+12] | ((uint16_t)cb[texmeta+13] << 8));
    wr16(cb + texmeta + 12, (uint16_t)(w * 2));
    assert(sa3ds_visual_preflight(&c, &t, &report) == SA3DS_VISUAL_METADATA_MISMATCH);
    wr16(cb + texmeta + 12, w);

    /* Reject invalid GPU dimensions independently of owner package sizes. */
    const size_t desc = 64;
    uint16_t tw = (uint16_t)(tb[desc] | ((uint16_t)tb[desc+1] << 8));
    wr16(tb + desc, 4);
    assert(sa3ds_visual_preflight(&c, &t, &report) == SA3DS_VISUAL_TEXTURE_DIMENSION);
    wr16(tb + desc, tw);

    /* A bad material slot must not result in an out-of-bounds GPU texture. */
    const size_t mat = c.sections[5].offset;
    uint16_t slot = (uint16_t)(cb[mat+12] | ((uint16_t)cb[mat+13] << 8));
    wr16(cb + mat + 12, (uint16_t)t.texture_count);
    assert(sa3ds_visual_preflight(&c, &t, &report) == SA3DS_VISUAL_MATERIAL_REFERENCE);
    wr16(cb + mat + 12, slot);

    uint32_t old_count = t.texture_count;
    t.texture_count = old_count - 1;
    assert(sa3ds_visual_preflight(&c, &t, &report) == SA3DS_VISUAL_COUNT_MISMATCH);
    t.texture_count = old_count;
    assert(sa3ds_visual_preflight(&c, &t, &report) == SA3DS_VISUAL_OK);

    /* Ensure alpha classification actually observes source pixels. */
    const size_t pixel = (size_t)(tb[desc+8] | ((uint32_t)tb[desc+9] << 8) |
                                   ((uint32_t)tb[desc+10] << 16) | ((uint32_t)tb[desc+11] << 24));
    uint8_t alpha = tb[pixel+3];
    wr32(tb + desc + 8, (uint32_t)pixel); /* preserve existing offset */
    tb[pixel+3] = 0;
    assert(sa3ds_visual_preflight(&c, &t, &report) == SA3DS_VISUAL_OK);
    assert(report.fully_transparent_texels > 0);
    tb[pixel+3] = alpha;

    printf("SA3DS Sonic visual GPU preflight: PASS (16 owner textures, 13 referenced, "
           "232192 bytes; metadata/dimension/material/alpha regressions)\n");
    free(tb); free(cb);
    return 0;
}
