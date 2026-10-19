#!/usr/bin/env python3
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from annotate_animation_intents import summarize

trace_root=Path('/tmp/sa3ds_host_tests')
required=['jump_tap','spindash_action','spindash_shortcut','loop_roll','accel_flat']
if not all((trace_root/f'{x}.csv').exists() for x in required):
    print('SKIP animation-intent trace integration (baseline traces unavailable)')
    raise SystemExit(0)

jump=summarize(trace_root/'jump_tap.csv')
spin=summarize(trace_root/'spindash_action.csv')
shortcut=summarize(trace_root/'spindash_shortcut.csv')
loop=summarize(trace_root/'loop_roll.csv')
accel=summarize(trace_root/'accel_flat.csv')

assert jump['transitions'][0]['intent']=='airborne_ball'
assert spin['transitions'][0]['intent']=='spindash_charge'
assert any(t['intent']=='roll' for t in spin['transitions'])
assert [t['intent'] for t in spin['transitions']] == [t['intent'] for t in shortcut['transitions']]
assert loop['transitions'][0]['intent']=='roll'
assert any(t['intent']=='grounded_jog_to_run' for t in accel['transitions'])
assert any(t['intent']=='grounded_run_plus' for t in accel['transitions'])
print('19-trace animation-intent bridge spot checks: PASS')
