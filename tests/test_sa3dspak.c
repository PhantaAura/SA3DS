#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "sa3dspak.h"

static unsigned char *read_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    if (fseek(file, 0, SEEK_END) || (*size = (size_t)ftell(file), fseek(file, 0, SEEK_SET))) return 0;
    unsigned char *data = (unsigned char *)malloc(*size ? *size : 1);
    if (!data || fread(data, 1, *size, file) != *size) return 0;
    fclose(file); return data;
}

int main(int argc, char **argv) {
    assert(argc == 3);
    size_t valid_size, bad_size;
    unsigned char *valid = read_file(argv[1], &valid_size);
    unsigned char *bad = read_file(argv[2], &bad_size);
    assert(valid && bad);
    SA3DSPak pak; SA3DSPakChunk metadata;
    assert(sa3dspak_open(&pak, valid, valid_size) == SA3DSPAK_OK);
    assert(pak.toc_count == 3);
    assert(sa3dspak_find(&pak, "METADATA", &metadata) == SA3DSPAK_OK);
    assert(metadata.size > 0);
    assert(sa3dspak_open(&pak, bad, bad_size) == SA3DSPAK_ERR_INTEGRITY);
    assert(sa3dspak_open(&pak, valid, 12) == SA3DSPAK_ERR_TRUNCATED);
    free(valid); free(bad);
    puts("runtime package reader tests: PASS");
    return 0;
}
