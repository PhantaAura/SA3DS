#!/usr/bin/env python3
"""Project regression expectations only; none are SADX reference values."""
import csv
import pathlib
import sys

def rows(root, name):
    with (root / (name + ".csv")).open(newline="") as stream: return list(csv.DictReader(stream))

def number(row, field): return float(row[field])

def main():
    root = pathlib.Path(sys.argv[1])
    accel = rows(root, "accel_flat"); coast = rows(root, "coast_from_speed")
    brake = rows(root, "brake_from_speed"); reverse = rows(root, "reverse_from_speed")
    tap = rows(root, "jump_tap"); hold = rows(root, "jump_hold"); reset = rows(root, "fall_reset")
    assert number(accel[-1], "horizontal_speed") > 0.4
    assert number(coast[-1], "horizontal_speed") < number(coast[59], "horizontal_speed")
    assert number(brake[89], "horizontal_speed") < number(brake[59], "horizontal_speed")
    assert min(number(row, "vel_z") for row in reverse[60:]) < -0.2
    assert max(number(row, "pos_y") for row in hold) > max(number(row, "pos_y") for row in tap)
    assert number(reset[0], "pos_z") == -32.0 and reset[0]["grounded"] == "1"
    assert any(row["surface_id"] == "2" for row in rows(root, "gentle_uphill"))
    assert any(row["surface_id"] == "3" for row in rows(root, "steep_uphill"))
    assert any(row["surface_id"] == "5" for row in rows(root, "bank_transition"))
    spin_a = rows(root, "spindash_action"); spin_s = rows(root, "spindash_shortcut")
    for a, b in zip(spin_a, spin_s):
        for field in ("pos_x", "pos_y", "pos_z", "vel_x", "vel_y", "vel_z",
                      "action_state", "ball_form", "spindash_charge", "spindash_ticks"):
            assert a[field] == b[field], (field, a[field], b[field])
    assert spin_a[0]["action_state"] == "2" and spin_a[0]["ball_form"] == "1"
    assert spin_a[20]["action_state"] == "3" and float(spin_a[20]["horizontal_speed"]) > 2.0
    loop = rows(root, "loop_roll")
    assert any(row["surface_id"] == "100" for row in loop)
    assert any(float(row["normal_y"]) < -0.80 for row in loop), "loop never reached ceiling-facing support"
    assert all(row["grounded"] == "1" for row in loop[:20]), "fast loop traversal detached before ceiling"
    slow_wall = rows(root, "loop_slow_wall_support")
    fast_wall = rows(root, "loop_fast_wall_stick")
    assert slow_wall[0]["grounded"] == "1", "unsupported 0.3/1.16 forced-fall heuristic is still active"
    assert fast_wall[0]["grounded"] == "1", "above-1.16 steep support detached"
    print("19 scenario regressions: PASS (flat/air/ball + 2004 support-order cases)")

if __name__ == "__main__": main()
