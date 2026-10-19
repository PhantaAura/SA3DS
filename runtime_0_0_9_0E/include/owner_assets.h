#ifndef SA3DS_0E_OWNER_ASSETS_H
#define SA3DS_0E_OWNER_ASSETS_H
#include <stddef.h>
#include <stdint.h>

int sa3ds_read_file(const char *path, uint8_t **out_data, size_t *out_size);
void sa3ds_free_file(void *data);

#endif
