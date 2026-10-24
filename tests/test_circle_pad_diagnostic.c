#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "../src/platform/3ds/input_adapter_math.h"

static int nearf(float a, float b) { return fabsf(a-b) < 1e-6f; }

int main(void) {
    SA3DSCirclePadSample s = sa3ds_circle_pad_map(0,0,156.0f);
    assert(nearf(s.normalized_x,0) && nearf(s.normalized_y,0));
    assert(nearf(s.normalized_magnitude,0) && s.normalized_angle_bams==0);

    s=sa3ds_circle_pad_map(1,-1,156.0f);
    assert(s.normalized_magnitude>0 && s.normalized_magnitude<0.01f);
    s=sa3ds_circle_pad_map(78,0,156.0f);
    assert(nearf(s.normalized_x,0.5f) && nearf(s.normalized_magnitude,0.5f));
    assert(s.normalized_angle_bams==0x4000);
    s=sa3ds_circle_pad_map(156,0,156.0f);
    assert(nearf(s.normalized_x,1) && nearf(s.normalized_magnitude,1));
    s=sa3ds_circle_pad_map(-200,200,156.0f);
    assert(nearf(s.normalized_x,-1) && nearf(s.normalized_y,1));
    assert(fabsf(s.normalized_magnitude-sqrtf(2.0f))<1e-6f);
    assert(s.normalized_angle_bams==0xe000);

    s=sa3ds_circle_pad_map(78,78,156.0f);
    assert(fabsf(s.normalized_magnitude-sqrtf(0.5f))<1e-6f);
    assert(s.normalized_angle_bams==0x2000);
    s=sa3ds_circle_pad_map(100,100,0.0f);
    assert(s.normalized_x==0 && s.normalized_y==0);
    puts("Circle Pad diagnostic mapping: PASS (synthetic raw samples; physical calibration NOT VERIFIED)");
    return 0;
}
