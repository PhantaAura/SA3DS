#ifndef SA3DS_SA3C_GPU_PREFLIGHT_H
#define SA3DS_SA3C_GPU_PREFLIGHT_H

#include <stdint.h>
#include "sa3c_character.h"
#include "sa3t_texture.h"

/* Host-testable, source-independent validation before PICA200 texture uploads.
 * No 3DS SDK or owner content is required to run these checks. */
typedef enum {
    SA3DS_VISUAL_OK = 0,
    SA3DS_VISUAL_ARGUMENT,
    SA3DS_VISUAL_COUNT_MISMATCH,
    SA3DS_VISUAL_TEXTURE_DIMENSION,
    SA3DS_VISUAL_METADATA_MISMATCH,
    SA3DS_VISUAL_TEXTURE_DATA,
    SA3DS_VISUAL_MATERIAL_REFERENCE
} SA3DSVisualResult;

typedef struct {
    uint32_t textures;
    uint32_t materials;
    uint32_t referenced_textures;
    uint64_t rgba8_bytes;
    uint64_t fully_transparent_texels;
    uint64_t partially_transparent_texels;
    uint64_t nonopaque_materials;
} SA3DSVisualReport;

SA3DSVisualResult sa3ds_visual_preflight(
    const SA3CCharacter *character, const SA3TArchive *textures,
    SA3DSVisualReport *report);
const char *sa3ds_visual_result_name(SA3DSVisualResult result);

/* SA3C geometry is already expanded to triangle-list indices by the
 * owner-side importer. The native draw path unconditionally uses
 * GPU_TRIANGLES and reads 8 float32 values per referenced vertex.
 * Validate that contract before submitting ANY GPU commands. This is a
 * format/safety gate, not proof of retail visual parity. */
typedef enum {
    SA3DS_GEOMETRY_OK = 0,
    SA3DS_GEOMETRY_ARGUMENT,
    SA3DS_GEOMETRY_SECTION,
    SA3DS_GEOMETRY_MESH_RANGE,
    SA3DS_GEOMETRY_PRIMITIVE,
    SA3DS_GEOMETRY_TRIANGLE_LIST,
    SA3DS_GEOMETRY_INDEX_RANGE,
    SA3DS_GEOMETRY_NONFINITE_VERTEX
} SA3DSGeometryResult;

typedef struct {
    uint32_t checked_meshes;
    uint32_t checked_indices;
    uint32_t checked_vertices;
    uint32_t triangle_count;
    uint32_t failing_mesh_index; /* UINT32_MAX when not mesh-specific */
    uint32_t failing_index_index; /* UINT32_MAX when not index-specific */
    uint32_t failing_vertex_index; /* UINT32_MAX when not vertex-specific */
    uint8_t failing_component; /* float index 0..7: position/normal/UV */
} SA3DSGeometryReport;

SA3DSGeometryResult sa3ds_geometry_preflight(
    const SA3CCharacter *character, SA3DSGeometryReport *report);
const char *sa3ds_geometry_result_name(SA3DSGeometryResult result);

/* NJS_MATERIAL diffuse is packed AARRGGBB in the owner-derived SA3C.
 * C3D_TexEnvColor uses the existing RRGGBBAA packing. Do not turn valid
 * black diffuse into white or discard the source material alpha. */
uint32_t sa3ds_visual_material_rgba(uint32_t diffuse_argb);

#endif
