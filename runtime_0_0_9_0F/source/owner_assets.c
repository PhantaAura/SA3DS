#include "owner_assets.h"
#include <stdio.h>
#include <stdlib.h>

int sa3ds_read_file(const char *path, uint8_t **out_data, size_t *out_size) {
    if (!path || !out_data || !out_size) return 0;
    *out_data = NULL; *out_size = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long n = ftell(f);
    if (n <= 0) { fclose(f); return 0; }
    rewind(f);
    uint8_t *p = (uint8_t*)malloc((size_t)n);
    if (!p) { fclose(f); return 0; }
    if (fread(p, 1, (size_t)n, f) != (size_t)n) { free(p); fclose(f); return 0; }
    fclose(f);
    *out_data = p; *out_size = (size_t)n;
    return 1;
}

void sa3ds_free_file(void *data) { free(data); }
