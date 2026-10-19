#include "sonic_action_preflight.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static float read_float(const uint8_t *p) {
    uint32_t bits = read_u32(p);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int finite_vec(SA3CVec3 v) {
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}

/* The three sampled poses cannot see a corrupt key hidden between their
 * sample positions. Validate EVERY active key in linear time before sampling.
 * Duplicate timestamps and end-of-clip keys are deliberately accepted until
 * their exact PC2004 semantics have been measured. Only descending key order
 * is rejected, because the linear sampler requires ordered timestamps. */
static SA3DSActionPreflightResult validate_tracks(
    const SA3CCharacter *c, const uint8_t *action,
    SA3DSActionPreflightReport *r) {
    const SA3CSection *chans = &c->sections[8];
    const SA3CSection *keys = &c->sections[9];
    uint32_t first = read_u32(action + 8), count = read_u32(action + 12);
    if (first > chans->count || count > chans->count - first)
        return SA3DS_ACTION_PREFLIGHT_TRACK_RANGE;
    for (uint32_t ci = first; ci < first + count; ++ci) {
        const uint8_t *ch = c->data + chans->offset + (size_t)ci * chans->stride;
        uint16_t flags = read_u16(ch + 2);
        const uint32_t starts[3] = {read_u32(ch + 4), read_u32(ch + 12), read_u32(ch + 16)};
        const uint16_t counts[3] = {read_u16(ch + 8), read_u16(ch + 10), read_u16(ch + 20)};
        for (unsigned t = 0; t < 3; ++t) {
            uint8_t track = (uint8_t)(1u << t);
            if (!(flags & track)) continue;
            r->failing_channel_index = ci;
            r->failing_track = track;
            r->failing_key_index = UINT32_MAX;
            r->failing_frame = 0.0f;
            if (!counts[t]) return SA3DS_ACTION_PREFLIGHT_TRACK_EMPTY;
            if (starts[t] > keys->count || counts[t] > keys->count - starts[t])
                return SA3DS_ACTION_PREFLIGHT_TRACK_RANGE;
            uint32_t last_frame = 0;
            for (uint32_t ki = 0; ki < counts[t]; ++ki) {
                const uint8_t *key = c->data + keys->offset +
                                     (size_t)(starts[t] + ki) * keys->stride;
                uint32_t frame = read_u32(key);
                r->failing_key_index = starts[t] + ki;
                r->failing_frame = (float)frame;
                if (ki && frame < last_frame)
                    return SA3DS_ACTION_PREFLIGHT_KEY_ORDER;
                if (!isfinite(read_float(key + 4)) ||
                    !isfinite(read_float(key + 8)) ||
                    !isfinite(read_float(key + 12)))
                    return SA3DS_ACTION_PREFLIGHT_NONFINITE;
                last_frame = frame;
            }
        }
    }
    r->failing_channel_index = UINT32_MAX;
    r->failing_key_index = UINT32_MAX;
    r->failing_track = 0;
    return SA3DS_ACTION_PREFLIGHT_OK;
}

SA3DSActionPreflightResult sa3ds_action_preflight(
    const SA3CCharacter *c,
    const SA3DSSonicAnimationBinding *bindings, size_t count,
    SA3DSActionPreflightReport *report) {
    SA3DSActionPreflightReport r = {0};
    r.failing_channel_index = UINT32_MAX;
    r.failing_key_index = UINT32_MAX;
    if (report) *report = r;
    if (!c || !c->data || !bindings || !report || !count ||
        !c->sections[0].count || c->sections[7].stride < 24 ||
        c->sections[8].stride < 24 || c->sections[9].stride < 16)
        return SA3DS_ACTION_PREFLIGHT_ARGUMENT;

    size_t nodes = c->sections[0].count;
    if (nodes > SIZE_MAX / sizeof(SA3CTransform) ||
        nodes > SIZE_MAX / sizeof(SA3CMatrix))
        return SA3DS_ACTION_PREFLIGHT_ARGUMENT;
    SA3CTransform *local = (SA3CTransform *)calloc(nodes, sizeof(*local));
    SA3CMatrix *world = (SA3CMatrix *)calloc(nodes, sizeof(*world));
    if (!local || !world) {
        free(local); free(world);
        return SA3DS_ACTION_PREFLIGHT_ARGUMENT;
    }
    SA3DSActionPreflightResult result = SA3DS_ACTION_PREFLIGHT_OK;
    for (size_t i = 0; i < count; ++i) {
        uint16_t slot = bindings[i].action_slot;
        r.failing_runtime_id = bindings[i].runtime_id;
        r.failing_action_slot = slot;
        r.failing_frame = 0.0f;
        if ((uint32_t)slot >= c->sections[7].count) {
            result = SA3DS_ACTION_PREFLIGHT_SLOT; break;
        }
        const uint8_t *a = c->data + c->sections[7].offset +
            (size_t)slot * c->sections[7].stride;
        uint16_t root = read_u16(a + 2);
        uint16_t frames = read_u16(a + 4);
        if (root == 0xFFFFu || frames == 0) {
            result = SA3DS_ACTION_PREFLIGHT_EMPTY; break;
        }
        if (root != bindings[i].source_root_node ||
            frames != bindings[i].source_frame_count) {
            result = SA3DS_ACTION_PREFLIGHT_METADATA; break;
        }
        result = validate_tracks(c, a, &r);
        if (result != SA3DS_ACTION_PREFLIGHT_OK) break;
        const float samples[3] = {0.0f, (float)(frames - 1u) * 0.5f,
                                  (float)(frames - 1u)};
        for (unsigned k = 0; k < 3; ++k) {
            r.failing_frame = samples[k];
            uint16_t root = 0xFFFFu;
            r.pose_error = sa3c_pose_sample_action(c, slot, samples[k], 0,
                                                   local, nodes, &root);
            if (r.pose_error != SA3C_POSE_OK || root == 0xFFFFu) {
                result = SA3DS_ACTION_PREFLIGHT_POSE; goto done;
            }
            for (size_t j = 0; j < nodes; ++j) {
                if (!finite_vec(local[j].position) ||
                    !finite_vec(local[j].rotation) ||
                    !finite_vec(local[j].scale)) {
                    result = SA3DS_ACTION_PREFLIGHT_NONFINITE; goto done;
                }
            }
            r.pose_error = sa3c_pose_build_world(c, root, local, nodes,
                                                 world, nodes);
            if (r.pose_error != SA3C_POSE_OK) {
                result = SA3DS_ACTION_PREFLIGHT_POSE; goto done;
            }
            /* Check every computed world matrix, not only the root: a
             * descendant can overflow even when all local values are finite.
             * Unvisited matrices remain zero-initialized and finite. */
            for (size_t j = 0; j < nodes; ++j) {
                for (unsigned m = 0; m < 16; ++m) {
                    if (!isfinite(world[j].m[m])) {
                        result = SA3DS_ACTION_PREFLIGHT_NONFINITE; goto done;
                    }
                }
            }
            ++r.checked_samples;
        }
        ++r.checked_bindings;
    }
done:
    free(world); free(local);
    *report = r;
    return result;
}

const char *sa3ds_action_preflight_result_name(SA3DSActionPreflightResult r) {
    switch (r) {
    case SA3DS_ACTION_PREFLIGHT_OK: return "OK";
    case SA3DS_ACTION_PREFLIGHT_ARGUMENT: return "ARGUMENT";
    case SA3DS_ACTION_PREFLIGHT_SLOT: return "SLOT";
    case SA3DS_ACTION_PREFLIGHT_EMPTY: return "EMPTY";
    case SA3DS_ACTION_PREFLIGHT_METADATA: return "METADATA";
    case SA3DS_ACTION_PREFLIGHT_POSE: return "POSE";
    case SA3DS_ACTION_PREFLIGHT_NONFINITE: return "NONFINITE";
    case SA3DS_ACTION_PREFLIGHT_TRACK_RANGE: return "TRACK_RANGE";
    case SA3DS_ACTION_PREFLIGHT_TRACK_EMPTY: return "TRACK_EMPTY";
    case SA3DS_ACTION_PREFLIGHT_KEY_ORDER: return "KEY_ORDER";
    default: return "UNKNOWN";
    }
}
