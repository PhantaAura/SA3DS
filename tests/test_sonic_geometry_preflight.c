/* Owner-free regression for the native GPU_TRIANGLES geometry contract. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/sa3c/sa3c_gpu_preflight.h"

static void wr16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void wr32(uint8_t *p, uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8u*i));
}
static void wrf(uint8_t *p, float v) {
    uint32_t bits; memcpy(&bits, &v, sizeof(bits)); wr32(p, bits);
}
typedef struct { uint8_t bytes[512]; SA3CCharacter c; } Fixture;
static void init(Fixture *f) {
    memset(f, 0, sizeof(*f));
    f->c.data = f->bytes; f->c.size = sizeof(f->bytes);
    f->c.sections[2] = (SA3CSection){{'M','E','S','H'}, 1, 64, 24, 24};
    f->c.sections[3] = (SA3CSection){{'V','R','T','X'}, 3, 128, 32, 96};
    f->c.sections[4] = (SA3CSection){{'I','N','D','X'}, 3, 256, 2, 6};
    wr32(f->bytes+64+4, 0); wr32(f->bytes+64+8, 3);
    wr16(f->bytes+64+12, 0xC000u);
    for (unsigned i = 0; i < 3; ++i) {
        wr16(f->bytes+256+2*i, (uint16_t)i);
        wrf(f->bytes+128+32*i, (float)i);
        wrf(f->bytes+128+32*i+20, 1.0f); /* normal z */
        wrf(f->bytes+128+32*i+24, (float)(i*128)); /* raw UV */
    }
}
int main(void) {
    Fixture f; SA3DSGeometryReport r;
    init(&f);
    assert(sa3ds_geometry_preflight(&f.c, &r) == SA3DS_GEOMETRY_OK);
    assert(r.checked_meshes == 1 && r.checked_indices == 3 &&
           r.checked_vertices == 3 && r.triangle_count == 1);
    assert(r.failing_mesh_index == UINT32_MAX);
    wrf(f.bytes+128+32+24, NAN);
    assert(sa3ds_geometry_preflight(&f.c, &r) == SA3DS_GEOMETRY_NONFINITE_VERTEX);
    assert(r.failing_mesh_index == 0 && r.failing_index_index == 1 &&
           r.failing_vertex_index == 1 && r.failing_component == 6);
    init(&f); wrf(f.bytes+128+2*32+12, INFINITY); /* normal x */
    assert(sa3ds_geometry_preflight(&f.c, &r) == SA3DS_GEOMETRY_NONFINITE_VERTEX);
    assert(r.failing_component == 3 && r.failing_vertex_index == 2);
    init(&f); wr16(f.bytes+256+4, 3);
    assert(sa3ds_geometry_preflight(&f.c, &r) == SA3DS_GEOMETRY_INDEX_RANGE);
    assert(r.failing_index_index == 2 && r.failing_vertex_index == 3);
    init(&f); wr16(f.bytes+64+12, 0);
    assert(sa3ds_geometry_preflight(&f.c, &r) == SA3DS_GEOMETRY_PRIMITIVE);
    init(&f); wr16(f.bytes+64+12, 0x4000u);
    assert(sa3ds_geometry_preflight(&f.c, &r) == SA3DS_GEOMETRY_OK);
    init(&f); wr32(f.bytes+64+8, 2);
    assert(sa3ds_geometry_preflight(&f.c, &r) == SA3DS_GEOMETRY_TRIANGLE_LIST);
    init(&f); wr32(f.bytes+64+8, 0); /* empty mesh allowed by renderer */
    assert(sa3ds_geometry_preflight(&f.c, &r) == SA3DS_GEOMETRY_OK);
    init(&f); wr32(f.bytes+64+4, 2); wr32(f.bytes+64+8, 3);
    assert(sa3ds_geometry_preflight(&f.c, &r) == SA3DS_GEOMETRY_MESH_RANGE);
    init(&f); f.c.sections[3].size = 31;
    assert(sa3ds_geometry_preflight(&f.c, &r) == SA3DS_GEOMETRY_SECTION);
    assert(sa3ds_geometry_preflight(NULL, &r) == SA3DS_GEOMETRY_ARGUMENT);
    assert(sa3ds_geometry_preflight(&f.c, NULL) == SA3DS_GEOMETRY_ARGUMENT);
    assert(strcmp(sa3ds_geometry_result_name(SA3DS_GEOMETRY_NONFINITE_VERTEX),
                  "NONFINITE_VERTEX") == 0);
    puts("SA3DS geometry preflight: PASS (triangle-list, primitive, finite vertex, bounds)");
    return 0;
}
