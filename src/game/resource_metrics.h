#ifndef SA3DS_GAME_RESOURCE_METRICS_H
#define SA3DS_GAME_RESOURCE_METRICS_H

#include <stddef.h>
#include <stdint.h>

enum {
    SA3DS_METRIC_STACK = 1u << 0,
    SA3DS_METRIC_HEAP = 1u << 1,
    SA3DS_METRIC_LINEAR = 1u << 2,
    SA3DS_METRIC_TEXTURE = 1u << 3
};

typedef struct {
    uint32_t known_mask;
    size_t stack_free, heap_free, linear_free, texture_bytes;
    size_t package_bytes, stage_bytes, transient_bytes;
} SA3DSResourceMetrics;

#endif
