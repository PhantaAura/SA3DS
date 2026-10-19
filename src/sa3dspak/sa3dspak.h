#ifndef SA3DSPAK_H
#define SA3DSPAK_H

#include <stddef.h>
#include <stdint.h>

#define SA3DSPAK_HEADER_SIZE 48u
#define SA3DSPAK_TOC_ENTRY_SIZE 40u
#define SA3DSPAK_VERSION_MAJOR 1u
#define SA3DSPAK_MAX_CHUNKS 4096u

typedef enum {
    SA3DSPAK_OK = 0,
    SA3DSPAK_ERR_ARGUMENT,
    SA3DSPAK_ERR_TRUNCATED,
    SA3DSPAK_ERR_MAGIC,
    SA3DSPAK_ERR_VERSION,
    SA3DSPAK_ERR_HEADER,
    SA3DSPAK_ERR_BOUNDS,
    SA3DSPAK_ERR_INTEGRITY,
    SA3DSPAK_ERR_NOT_FOUND
} SA3DSPakResult;

typedef struct {
    const uint8_t *data;
    size_t size;
    uint32_t toc_count;
    uint64_t toc_offset;
} SA3DSPak;

typedef struct {
    char type[17];
    const uint8_t *data;
    uint64_t size;
    uint32_t flags;
} SA3DSPakChunk;

SA3DSPakResult sa3dspak_open(SA3DSPak *pak, const void *data, size_t size);
SA3DSPakResult sa3dspak_chunk_at(const SA3DSPak *pak, uint32_t index, SA3DSPakChunk *chunk);
SA3DSPakResult sa3dspak_find(const SA3DSPak *pak, const char *type, SA3DSPakChunk *chunk);
const char *sa3dspak_result_string(SA3DSPakResult result);

#endif
