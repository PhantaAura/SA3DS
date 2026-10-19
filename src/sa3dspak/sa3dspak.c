#include "sa3dspak.h"
#include <string.h>

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}
static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t read_u64(const uint8_t *p) {
    return (uint64_t)read_u32(p) | ((uint64_t)read_u32(p + 4) << 32);
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t size) {
    crc = ~crc;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (uint32_t)-(int32_t)(crc & 1u));
    }
    return ~crc;
}

static uint32_t header_crc(const uint8_t *header) {
    uint8_t copy[SA3DSPAK_HEADER_SIZE];
    memcpy(copy, header, sizeof(copy));
    memset(copy + 44, 0, 4);
    return crc32_update(0, copy, sizeof(copy));
}

SA3DSPakResult sa3dspak_open(SA3DSPak *pak, const void *data_void, size_t size) {
    if (!pak || !data_void) return SA3DSPAK_ERR_ARGUMENT;
    memset(pak, 0, sizeof(*pak));
    const uint8_t *data = (const uint8_t *)data_void;
    if (size < SA3DSPAK_HEADER_SIZE) return SA3DSPAK_ERR_TRUNCATED;
    if (memcmp(data, "SA3DSPAK", 8) != 0) return SA3DSPAK_ERR_MAGIC;
    if (read_u16(data + 8) != SA3DSPAK_VERSION_MAJOR) return SA3DSPAK_ERR_VERSION;
    if (read_u32(data + 12) != SA3DSPAK_HEADER_SIZE) return SA3DSPAK_ERR_HEADER;
    if (read_u64(data + 16) != (uint64_t)size) return SA3DSPAK_ERR_BOUNDS;
    uint64_t toc_offset = read_u64(data + 24);
    uint32_t toc_count = read_u32(data + 32);
    if (toc_count > SA3DSPAK_MAX_CHUNKS) return SA3DSPAK_ERR_BOUNDS;
    if (toc_offset < SA3DSPAK_HEADER_SIZE || toc_offset > size) return SA3DSPAK_ERR_BOUNDS;
    uint64_t toc_bytes = (uint64_t)toc_count * SA3DSPAK_TOC_ENTRY_SIZE;
    if (toc_bytes > (uint64_t)size - toc_offset) return SA3DSPAK_ERR_BOUNDS;
    if (read_u32(data + 44) != header_crc(data)) return SA3DSPAK_ERR_INTEGRITY;
    if (read_u32(data + 40) != crc32_update(0, data + SA3DSPAK_HEADER_SIZE,
                                            size - SA3DSPAK_HEADER_SIZE))
        return SA3DSPAK_ERR_INTEGRITY;

    for (uint32_t i = 0; i < toc_count; ++i) {
        const uint8_t *entry = data + toc_offset + (uint64_t)i * SA3DSPAK_TOC_ENTRY_SIZE;
        uint64_t offset = read_u64(entry + 16);
        uint64_t chunk_size = read_u64(entry + 24);
        if (offset < toc_offset + toc_bytes || offset > size || chunk_size > (uint64_t)size - offset)
            return SA3DSPAK_ERR_BOUNDS;
        if (read_u32(entry + 32) != crc32_update(0, data + offset, (size_t)chunk_size))
            return SA3DSPAK_ERR_INTEGRITY;
    }
    pak->data = data; pak->size = size; pak->toc_count = toc_count; pak->toc_offset = toc_offset;
    return SA3DSPAK_OK;
}

SA3DSPakResult sa3dspak_chunk_at(const SA3DSPak *pak, uint32_t index, SA3DSPakChunk *chunk) {
    if (!pak || !chunk || !pak->data) return SA3DSPAK_ERR_ARGUMENT;
    if (index >= pak->toc_count) return SA3DSPAK_ERR_NOT_FOUND;
    const uint8_t *entry = pak->data + pak->toc_offset + (uint64_t)index * SA3DSPAK_TOC_ENTRY_SIZE;
    memcpy(chunk->type, entry, 16); chunk->type[16] = '\0';
    chunk->data = pak->data + read_u64(entry + 16);
    chunk->size = read_u64(entry + 24);
    chunk->flags = read_u32(entry + 36);
    return SA3DSPAK_OK;
}

SA3DSPakResult sa3dspak_find(const SA3DSPak *pak, const char *type, SA3DSPakChunk *chunk) {
    if (!type) return SA3DSPAK_ERR_ARGUMENT;
    for (uint32_t i = 0; i < pak->toc_count; ++i) {
        SA3DSPakResult result = sa3dspak_chunk_at(pak, i, chunk);
        if (result != SA3DSPAK_OK) return result;
        if (strncmp(chunk->type, type, 16) == 0) return SA3DSPAK_OK;
    }
    return SA3DSPAK_ERR_NOT_FOUND;
}

const char *sa3dspak_result_string(SA3DSPakResult result) {
    static const char *messages[] = {
        "ok", "invalid argument", "truncated", "bad magic", "unsupported version",
        "invalid header", "out of bounds", "integrity check failed", "not found"
    };
    return (unsigned)result < sizeof(messages)/sizeof(messages[0]) ? messages[result] : "unknown error";
}
