#include "sa3c_gpu_preflight.h"

#include <stddef.h>
#include <math.h>
#include <string.h>

static uint16_t r16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t r32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int gpu_dimension_ok(uint16_t v) {
    return v >= 8 && v <= 1024 && (v & (uint16_t)(v - 1)) == 0;
}
uint32_t sa3ds_visual_material_rgba(uint32_t argb) {
    return ((argb & 0x00FFFFFFu) << 8) | (argb >> 24);
}

SA3DSVisualResult sa3ds_visual_preflight(
    const SA3CCharacter *c, const SA3TArchive *t, SA3DSVisualReport *report) {
    SA3DSVisualReport result = {0};
    if (!c || !c->data || !t || !t->data || !report) return SA3DS_VISUAL_ARGUMENT;
    memset(report, 0, sizeof(*report));
    const SA3CSection *texs = &c->sections[6];
    const SA3CSection *mats = &c->sections[5];
    if (!t->texture_count || t->texture_count != texs->count)
        return SA3DS_VISUAL_COUNT_MISMATCH;
    if (texs->stride < 24 || mats->stride < 28)
        return SA3DS_VISUAL_ARGUMENT;
    /* Distinct texture slots are bounded by the owner archive. A count does
     * not require heap allocation or a VLA on the Old 3DS stack. */
    for (uint32_t i = 0; i < t->texture_count; ++i) {
        SA3TTexture desc;
        const uint8_t *pixels = NULL;
        if (sa3t_texture(t, i, &desc, &pixels) != SA3T_OK || !pixels)
            return SA3DS_VISUAL_TEXTURE_DATA;
        if (!gpu_dimension_ok(desc.width) || !gpu_dimension_ok(desc.height))
            return SA3DS_VISUAL_TEXTURE_DIMENSION;
        const uint8_t *meta = c->data + texs->offset + (size_t)i * texs->stride;
        /* SA3C TEXS: index, global index, pixel/data format, width, height.
         * SA3T keeps these fields separately from decoded RGBA pixels. */
        if (r32(meta) != i || r32(meta + 4) != desc.global_index ||
            r16(meta + 8) != desc.source_pixel_format ||
            r16(meta + 10) != desc.source_data_format ||
            r16(meta + 12) != desc.width || r16(meta + 14) != desc.height)
            return SA3DS_VISUAL_METADATA_MISMATCH;
        result.rgba8_bytes += desc.data_size;
        for (uint32_t p = 3; p < desc.data_size; p += 4) {
            if (pixels[p] == 0) ++result.fully_transparent_texels;
            else if (pixels[p] != 255) ++result.partially_transparent_texels;
        }
    }
    for (uint32_t i = 0; i < mats->count; ++i) {
        const uint8_t *mat = c->data + mats->offset + (size_t)i * mats->stride;
        const uint16_t slot = r16(mat + 12);
        if (slot >= t->texture_count) return SA3DS_VISUAL_MATERIAL_REFERENCE;
        if ((r32(mat) >> 24) != 255) ++result.nonopaque_materials;
        int first = 1;
        for (uint32_t j = 0; j < i; ++j) {
            const uint8_t *prior = c->data + mats->offset + (size_t)j * mats->stride;
            if (r16(prior + 12) == slot) { first = 0; break; }
        }
        if (first) ++result.referenced_textures;
    }
    result.textures = t->texture_count;
    result.materials = mats->count;
    *report = result;
    return SA3DS_VISUAL_OK;
}

const char *sa3ds_visual_result_name(SA3DSVisualResult result) {
    switch (result) {
        case SA3DS_VISUAL_OK: return "OK";
        case SA3DS_VISUAL_ARGUMENT: return "INVALID_ARGUMENT_OR_SECTION";
        case SA3DS_VISUAL_COUNT_MISMATCH: return "TEXTURE_COUNT_MISMATCH";
        case SA3DS_VISUAL_TEXTURE_DIMENSION: return "INVALID_PICA_TEXTURE_DIMENSION";
        case SA3DS_VISUAL_METADATA_MISMATCH: return "SA3C_SA3T_METADATA_MISMATCH";
        case SA3DS_VISUAL_TEXTURE_DATA: return "INVALID_TEXTURE_DATA";
        case SA3DS_VISUAL_MATERIAL_REFERENCE: return "INVALID_MATERIAL_TEXTURE_SLOT";
        default: return "UNKNOWN_VISUAL_ERROR";
    }
}

/* Native Sonic renderer assumes the owner-side SA3C builder has already
 * expanded strips/quads to triangle lists. Invalid input must not reach the
 * PICA200 vertex fetcher. This walks only the package, never owner binaries. */
SA3DSGeometryResult sa3ds_geometry_preflight(
    const SA3CCharacter *c, SA3DSGeometryReport *report) {
    if (!report) return SA3DS_GEOMETRY_ARGUMENT;
    memset(report, 0, sizeof(*report));
    report->failing_mesh_index = UINT32_MAX;
    report->failing_index_index = UINT32_MAX;
    report->failing_vertex_index = UINT32_MAX;
    if (!c || !c->data) return SA3DS_GEOMETRY_ARGUMENT;
    const SA3CSection *mesh = &c->sections[2];
    const SA3CSection *vertex = &c->sections[3];
    const SA3CSection *index = &c->sections[4];
    if (mesh->stride < 16 || vertex->stride < 32 || index->stride < 2 ||
        mesh->count == 0 || vertex->count == 0 || index->count == 0)
        return SA3DS_GEOMETRY_SECTION;
    if ((size_t)mesh->offset > c->size || mesh->size > c->size - (size_t)mesh->offset ||
        (size_t)vertex->offset > c->size || vertex->size > c->size - (size_t)vertex->offset ||
        (size_t)index->offset > c->size || index->size > c->size - (size_t)index->offset ||
        mesh->count > mesh->size / mesh->stride ||
        vertex->count > vertex->size / vertex->stride ||
        index->count > index->size / index->stride)
        return SA3DS_GEOMETRY_SECTION;
    for (uint32_t mi = 0; mi < mesh->count; ++mi) {
        const uint8_t *m = c->data + mesh->offset + (size_t)mi * mesh->stride;
        uint32_t first = r32(m + 4), count = r32(m + 8);
        uint16_t primitive = r16(m + 12);
        report->failing_mesh_index = mi;
        if (first > index->count || count > index->count - first)
            return SA3DS_GEOMETRY_MESH_RANGE;
        if (primitive != 0xC000u && primitive != 0x4000u)
            return SA3DS_GEOMETRY_PRIMITIVE;
        if (count % 3u)
            return SA3DS_GEOMETRY_TRIANGLE_LIST;
        for (uint32_t k = 0; k < count; ++k) {
            uint32_t ii = first + k;
            uint32_t vi = r16(c->data + index->offset + (size_t)ii * index->stride);
            report->failing_index_index = ii;
            report->failing_vertex_index = vi;
            if (vi >= vertex->count) return SA3DS_GEOMETRY_INDEX_RANGE;
            const uint8_t *v = c->data + vertex->offset + (size_t)vi * vertex->stride;
            for (unsigned component = 0; component < 8; ++component) {
                uint32_t bits = r32(v + component * 4u);
                float value;
                memcpy(&value, &bits, sizeof(value));
                report->failing_component = (uint8_t)component;
                if (!isfinite(value)) return SA3DS_GEOMETRY_NONFINITE_VERTEX;
            }
            ++report->checked_vertices; /* referenced instances, not unique */
        }
        report->checked_indices += count;
        report->triangle_count += count / 3u;
        ++report->checked_meshes;
    }
    report->failing_mesh_index = UINT32_MAX;
    report->failing_index_index = UINT32_MAX;
    report->failing_vertex_index = UINT32_MAX;
    report->failing_component = 0;
    return SA3DS_GEOMETRY_OK;
}

const char *sa3ds_geometry_result_name(SA3DSGeometryResult result) {
    switch (result) {
    case SA3DS_GEOMETRY_OK: return "OK";
    case SA3DS_GEOMETRY_ARGUMENT: return "ARGUMENT";
    case SA3DS_GEOMETRY_SECTION: return "SECTION";
    case SA3DS_GEOMETRY_MESH_RANGE: return "MESH_RANGE";
    case SA3DS_GEOMETRY_PRIMITIVE: return "PRIMITIVE";
    case SA3DS_GEOMETRY_TRIANGLE_LIST: return "TRIANGLE_LIST";
    case SA3DS_GEOMETRY_INDEX_RANGE: return "INDEX_RANGE";
    case SA3DS_GEOMETRY_NONFINITE_VERTEX: return "NONFINITE_VERTEX";
    default: return "UNKNOWN";
    }
}
