#include "resource_metrics_3ds.h"
#include <3ds.h>

void sa3ds_query_resource_metrics(SA3DSResourceMetrics *metrics) {
    if (!metrics) return;
    metrics->known_mask |= SA3DS_METRIC_LINEAR;
    metrics->linear_free = linearSpaceFree();
    /* Stack/heap/texture figures remain unknown until a validated collector exists. */
}
