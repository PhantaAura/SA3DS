#!/usr/bin/env python3
"""Observational PC2004 retail trace launcher for compatible Windows x86.

The owner's executable is fingerprinted and never modified. Frida is imported
only for a live capture; --self-test remains owner-data-free and portable.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import platform
import sys
import threading
from datetime import datetime, timezone
from pathlib import Path

EXPECTED_SHA="6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"
SCENARIOS=("neutral_standing","partial_acceleration","full_acceleration",
           "release_coasting","direction_reversal","running_jump_landing",
           "spindash_charge_release")
FIELDS=("trace_version","test_id","test_name","tick_phase","sonic_act1_ran",
        "frame","move_x","move_z",
        "filtered_input_magnitude","filtered_input_angle_bams","camera_yaw",
        "circle_raw_x","circle_raw_y","pos_x","pos_y","pos_z",
        "vel_x","vel_y","vel_z","local_forward_speed","local_side_speed",
        "heading","grounded","action_state","status_bits","player_status_bits",
        "accel_forward","accel_normal","accel_side","spindash_charge",
        "spindash_ticks","animation_requested_id","runtime_animation_id",
        "animation_frame","animation_transition_state","animation_transition_source_id")
OBSERVED_FIELDS=[x for x in FIELDS if x not in ("test_name",)]

def sha256(path:Path)->str:
    h=hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda:f.read(1024*1024),b""): h.update(block)
    return h.hexdigest()

def write_capture(rows:list[dict],output:Path,scenario:str,live:bool,extra:dict|None=None):
    output.parent.mkdir(parents=True,exist_ok=True)
    test_id=SCENARIOS.index(scenario)+1
    with output.open("w",newline="",encoding="utf-8") as f:
        writer=csv.DictWriter(f,fieldnames=FIELDS,extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            row=dict(row); row.update(test_id=test_id,test_name=scenario)
            writer.writerow(row)
    meta={
        "schema":"sa3ds-pc2004-retail-capture-v2",
        "authority":"RETAIL TRACE VERIFIED" if live else "NOT VERIFIED",
        "executable_sha256":EXPECTED_SHA,
        "runtime_capture":bool(live),"synthetic":not live,"observational":True,
        "scenario_id":scenario,
        "capture_utc":datetime.now(timezone.utc).isoformat().replace("+00:00","Z"),
        "trace_sha256":sha256(output),"observed_fields":OBSERVED_FIELDS,
        "coordinate_contract":{
            "units":"native PC2004 stage units",
            "position":"EntityData1.Position x/y/z",
            "world_velocity":"motion-work offsets +0x04/+0x08/+0x0c (x/y/z)",
            "local_velocity":"CharObj2 Speed.x/Speed.z",
            "angles":"PC2004 BAMS converted to radians",
            "sa3ds_mapping":"identity pending matched-spawn confirmation"
        },
        "tick_contract":"one row after each Sonic main return, after movement and animation dispatch; alignment to SA3DS must still be measured",
        "host":{"system":platform.system(),"release":platform.release(),"machine":platform.machine()},
    }
    if extra: meta.update(extra)
    sidecar=output.with_suffix(output.suffix+".json")
    sidecar.write_text(json.dumps(meta,indent=2,sort_keys=True)+"\n",encoding="utf-8")
    return sidecar

def synthetic_rows():
    return [{"trace_version":3,"tick_phase":"post_sonic_main","sonic_act1_ran":1,
             "frame":i,"move_x":0.0,"move_z":0.0,
             "filtered_input_magnitude":0.0,"filtered_input_angle_bams":0,
             "camera_yaw":0.0,"circle_raw_x":0,"circle_raw_y":0,
             "pos_x":0.0,"pos_y":0.0,"pos_z":0.0,"vel_x":0.0,"vel_y":0.0,
             "vel_z":0.0,"local_forward_speed":0.0,"local_side_speed":0.0,
             "heading":0.0,"grounded":1,"action_state":1,"status_bits":3,
             "player_status_bits":0,"accel_forward":0.0,"accel_normal":0.0,
             "accel_side":0.0,"spindash_charge":0.0,"spindash_ticks":0,
             "animation_requested_id":0,"runtime_animation_id":0,
             "animation_frame":0.0,"animation_transition_state":0,
             "animation_transition_source_id":0} for i in range(3)]

def live_capture(args):
    exe=args.exe.resolve()
    actual=sha256(exe)
    if actual!=EXPECTED_SHA:
        raise SystemExit(f"unsupported executable sha256={actual}")
    try:
        import frida
    except ImportError as e:
        raise SystemExit("Frida Python package missing; install frida-tools in Windows x86 Python") from e
    agent=Path(__file__).with_name("pc2004_trace_agent.js").read_text(encoding="utf-8")
    rows=[]; errors=[]; ready={}; done=threading.Event()
    device=frida.get_local_device()
    old_cwd=os.getcwd(); pid=None
    try:
        if args.pid:
            pid=args.pid
        else:
            os.chdir(str(exe.parent))
            pid=device.spawn([str(exe)])
        session=device.attach(pid)
        script=session.create_script(agent)
        def message(msg,data):
            if msg.get("type")!="send":
                errors.append(str(msg)); done.set(); return
            payload=msg.get("payload",{})
            kind=payload.get("kind")
            if kind=="row":
                rows.append(payload["row"])
                if args.frames and len(rows)>=args.frames: done.set()
            elif kind=="ready": ready.update(payload)
            elif kind=="capture_error": errors.append(json.dumps(payload,sort_keys=True))
        script.on("message",message); script.load()
        if not args.pid: device.resume(pid)
        print(f"capture ready pid={pid}; run scenario {args.scenario}; Ctrl+C stops",flush=True)
        try:
            while not done.wait(0.25): pass
        except KeyboardInterrupt:
            pass
        script.unload(); session.detach()
    finally:
        os.chdir(old_cwd)
    if not ready:
        raise SystemExit("capture agent did not report a verified ready state")
    if errors:
        raise SystemExit("capture aborted after instrumentation error(s): " + "; ".join(errors))
    if not rows: raise SystemExit("no post-Sonic-main rows captured")
    sidecar=write_capture(rows,args.output,args.scenario,True,{
        "capture_tool":"sa3ds-pc2004-frida-v2",
        "frida_version":getattr(frida,"__version__","unknown"),
        "pid":pid,"agent_ready":ready,"capture_errors":errors,
        "exe_path_recorded_locally":str(exe),
    })
    print(f"wrote {len(rows)} rows: {args.output}\nmetadata: {sidecar}")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--exe",type=Path)
    ap.add_argument("--pid",type=int,help="attach to an already running exact executable")
    ap.add_argument("--output",type=Path,default=Path("pc2004_trace.csv"))
    ap.add_argument("--scenario",choices=SCENARIOS,default=SCENARIOS[0])
    ap.add_argument("--frames",type=int,default=600)
    ap.add_argument("--self-test",action="store_true")
    args=ap.parse_args()
    if args.self_test:
        sidecar=write_capture(synthetic_rows(),args.output,args.scenario,False,
                              {"capture_tool":"sa3ds-pc2004-frida-v2-self-test"})
        meta=json.loads(sidecar.read_text())
        assert meta["authority"]=="NOT VERIFIED" and meta["synthetic"]
        assert meta["trace_sha256"]==sha256(args.output)
        print(f"capture schema self-test: PASS ({len(FIELDS)} columns; synthetic/NOT VERIFIED)")
        return 0
    if not args.exe: ap.error("--exe is required for live capture")
    return live_capture(args) or 0

if __name__=="__main__": raise SystemExit(main())
