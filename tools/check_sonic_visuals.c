/* Read-only, owner-side SA3C + SA3T visual and geometry validation.
 * No proprietary game data is embedded or written to output. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/sa3c/sa3c_character.h"
#include "../src/sa3c/sa3t_texture.h"
#include "../src/sa3c/sa3c_gpu_preflight.h"

#define MAX_OWNER_PACKAGE_BYTES (128u * 1024u * 1024u)

static uint8_t *read_file(const char *path, size_t *length) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return NULL; }
    if (fseek(f, 0, SEEK_END)) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size <= 0 || (unsigned long)size > MAX_OWNER_PACKAGE_BYTES ||
        fseek(f, 0, SEEK_SET)) {
        fprintf(stderr, "invalid or oversized owner package: %s\n", path);
        fclose(f); return NULL;
    }
    uint8_t *data = malloc((size_t)size);
    if (!data) { fprintf(stderr, "out of memory: %s\n", path); fclose(f); return NULL; }
    if (fread(data, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "short read: %s\n", path);
        free(data); fclose(f); return NULL;
    }
    fclose(f);
    *length = (size_t)size;
    return data;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s owner/sonic.sa3c owner/sonic.sa3t\n", argv[0]);
        return 2;
    }
    size_t nc = 0, nt = 0;
    uint8_t *bc = read_file(argv[1], &nc);
    if (!bc) return 2;
    uint8_t *bt = read_file(argv[2], &nt);
    if (!bt) { free(bc); return 2; }
    SA3CCharacter c;
    SA3TArchive t;
    SA3CResult cr = sa3c_character_open(&c, bc, nc);
    if (cr == SA3C_OK) cr = sa3c_character_validate_references(&c);
    if (cr != SA3C_OK) {
        fprintf(stderr, "SA3C format/references: %s\n", sa3c_result_name(cr));
        free(bt); free(bc); return 1;
    }
    SA3TResult tr = sa3t_open(&t, bt, nt);
    if (tr != SA3T_OK) {
        fprintf(stderr, "SA3T format: %s\n", sa3t_result_name(tr));
        free(bt); free(bc); return 1;
    }
    /* Do NOT compare SA3C and SA3T source hashes: SA3C is derived from
     * CHRMODELS_orig.dll and SA3T from SONIC.PVM. Their hashes are expected
     * to differ. Compatibility is checked through TEXS/SA3T metadata below. */
    SA3DSVisualReport vr;
    SA3DSVisualResult v = sa3ds_visual_preflight(&c, &t, &vr);
    if (v != SA3DS_VISUAL_OK) {
        fprintf(stderr, "visual preflight: %s\n", sa3ds_visual_result_name(v));
        free(bt); free(bc); return 1;
    }
    SA3DSGeometryReport gr;
    SA3DSGeometryResult g = sa3ds_geometry_preflight(&c, &gr);
    if (g != SA3DS_GEOMETRY_OK) {
        fprintf(stderr, "geometry preflight: %s mesh=%lu index=%lu vertex=%lu component=%u\n",
            sa3ds_geometry_result_name(g), (unsigned long)gr.failing_mesh_index,
            (unsigned long)gr.failing_index_index, (unsigned long)gr.failing_vertex_index,
            (unsigned)gr.failing_component);
        free(bt); free(bc); return 1;
    }
    printf("SA3DS owner visuals PASS: textures=%lu materials=%lu referenced=%lu "
           "rgba_bytes=%llu alpha_zero=%llu alpha_partial=%llu "
           "meshes=%lu triangles=%lu checked_indices=%lu\n",
           (unsigned long)vr.textures, (unsigned long)vr.materials,
           (unsigned long)vr.referenced_textures, (unsigned long long)vr.rgba8_bytes,
           (unsigned long long)vr.fully_transparent_texels,
           (unsigned long long)vr.partially_transparent_texels,
           (unsigned long)gr.checked_meshes, (unsigned long)gr.triangle_count,
           (unsigned long)gr.checked_indices);
    free(bt); free(bc);
    return 0;
}
