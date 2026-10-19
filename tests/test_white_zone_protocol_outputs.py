#!/usr/bin/env python3
import csv, pathlib, sys

root=pathlib.Path(sys.argv[1])

def rows(name):
    with open(root/f"{name}.csv",newline="",encoding="utf-8") as f: return list(csv.DictReader(f))
def fl(r,k): return float(r[k])
def ints(rs,k): return {int(r[k]) for r in rs}

acc=rows("flat_accel"); assert fl(acc[-1],"horizontal_speed")>fl(acc[0],"horizontal_speed")
coast=rows("coast"); assert fl(coast[-1],"horizontal_speed")<fl(coast[0],"horizontal_speed")
brake=rows("brake"); assert max(abs(fl(r,"heading")) for r in brake)>1.0
for n in ("turn_low","turn_high"):
    rs=rows(n); assert max(abs(fl(r,"heading")) for r in rs)>0.5

def jump_stats(name):
    rs=rows(name); assert 0 in ints(rs,"grounded") and 1 in ints(rs,"grounded")
    apex=max(range(len(rs)),key=lambda i: fl(rs[i],"pos_y"))
    airborne_seen=False; landing=None
    for i,r in enumerate(rs):
        if int(r["grounded"])==0: airborne_seen=True
        elif airborne_seen: landing=i; break
    assert landing is not None and landing>apex
    return max(fl(r,"pos_y") for r in rs),apex,landing

tap=jump_stats("jump_tap"); hold=jump_stats("jump_hold"); assert hold[0]>tap[0]
spin=rows("spindash"); assert 2 in ints(spin,"action_state") and 3 in ints(spin,"action_state")
roll=rows("roll_exit"); assert fl(roll[-1],"horizontal_speed")<=fl(roll[0],"horizontal_speed")
for n in ("uphill","downhill"):
    rs=rows(n); assert rs and all(r["selector_provisional"]=="1" for r in rs)
air=rows("air_control"); assert 0 in ints(air,"grounded") and max(abs(fl(r,"heading")) for r in air)>0.5
print("12 deterministic White Zone protocol behavior checks: PASS")
print(f"jump_tap apex_index={tap[1]} landing_index={tap[2]}")
print(f"jump_hold apex_index={hold[1]} landing_index={hold[2]}")
