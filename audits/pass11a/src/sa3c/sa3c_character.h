#ifndef SA3DS_SA3C_CHARACTER_H
#define SA3DS_SA3C_CHARACTER_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    SA3C_OK = 0,
    SA3C_ERR_ARGUMENT,
    SA3C_ERR_TRUNCATED,
    SA3C_ERR_MAGIC,
    SA3C_ERR_VERSION,
    SA3C_ERR_HEADER,
    SA3C_ERR_SECTION,
    SA3C_ERR_RANGE,
    SA3C_ERR_REFERENCE
} SA3CResult;

typedef struct {
    char tag[4];
    uint32_t count;
    uint32_t offset;
    uint32_t stride;
    uint32_t size;
} SA3CSection;

typedef struct {
    const uint8_t *data;
    size_t size;
    uint32_t flags;
    uint8_t source_sha256[32];
    SA3CSection sections[10];
} SA3CCharacter;

SA3CResult sa3c_character_open(SA3CCharacter *out, const void *data, size_t size);
SA3CResult sa3c_character_validate_references(const SA3CCharacter *c);
const SA3CSection *sa3c_character_section(const SA3CCharacter *c, const char tag[4]);
const void *sa3c_character_section_data(const SA3CCharacter *c, const SA3CSection *s);
const char *sa3c_result_name(SA3CResult result);

#endif
