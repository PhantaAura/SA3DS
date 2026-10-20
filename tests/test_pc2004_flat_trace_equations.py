#!/usr/bin/env python3
"""Check enriched White Zone rows against exact PC2004 flat equations.

These are source-equation fixtures, not captured retail runtime traces.
"""
import csv
import math
import pathlib
import sys

root=pathlib.Path(sys.argv[1])

def rows(name):
    with (root/f"{name}.csv").open(newline="",encoding="utf-8") as f:
        return list(csv.DictReader(f))

def value(row,name): return float(row[name])

neutral=rows("neutral")
assert all(value(r,"filtered_input_magnitude")==0.0 for r in neutral)
assert all(value(r,"requested_force")==0.0 for r in neutral)

partial=rows("partial")[0]
assert math.isclose(value(partial,"filtered_input_magnitude"),0.125,abs_tol=2e-5)
assert math.isclose(value(partial,"requested_force"),0.125*0.05,abs_tol=2e-6)
assert math.isclose(value(partial,"local_forward_after"),
                    value(partial,"accel_forward"),abs_tol=2e-6)

quick=rows("quick_turn")[0]
assert int(quick["turn_kind"])==2
assert int(quick["angle_delta_bams"])==0x4000
assert int(quick["turn_step_bams"])==0x2000
assert math.isclose(value(quick,"heading"),math.pi/4,abs_tol=2e-6)

camera=rows("camera_turn")[0]
assert math.isclose(value(camera,"camera_yaw"),math.pi/2,abs_tol=2e-6)
assert int(camera["turn_kind"])==2
assert int(camera["angle_delta_bams"])==0x4000
assert int(camera["turn_step_bams"])==0x2000
assert math.isclose(value(camera,"heading"),math.pi/4,abs_tol=2e-6)

side=rows("side_threshold")
assert math.isclose(value(side[0],"local_side_before"),1.0,abs_tol=1e-6)
assert math.isclose(value(side[0],"accel_side"),-0.4,abs_tol=1e-6)
assert math.isclose(value(side[0],"local_side_after"),0.6,abs_tol=1e-6)
assert int(side[0]["side_stopped_at_jog_threshold"])==0
assert math.isclose(value(side[1],"local_side_before"),0.6,abs_tol=1e-6)
assert math.isclose(value(side[1],"accel_side"),-0.24,abs_tol=1e-6)
assert math.isclose(value(side[1],"local_side_after"),0.0,abs_tol=1e-6)
assert int(side[1]["side_stopped_at_jog_threshold"])==1

print("SOURCE-EQUATION VERIFIED: neutral/partial/quick/camera-relative/local-Z deadband rows")
