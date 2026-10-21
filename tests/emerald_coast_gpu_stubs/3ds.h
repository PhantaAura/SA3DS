#ifndef SA3DS_TEST_3DS_H
#define SA3DS_TEST_3DS_H
/* Host-only SDK boundary. These stubs never exercise real GPU hardware. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
void *linearAlloc(size_t size);
void linearFree(void *data);
void GSPGPU_FlushDataCache(const void *data,size_t size);
#endif
