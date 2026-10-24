#!/usr/bin/env python3
"""Strict White Zone reference-vs-SA3DS frame comparator.

The caller must identify reference authority. Agreement never promotes the
global project beyond the fields and frames present in the two traces.
"""
import argparse
import csv
import hashlib
import json
import math
from collections import Counter

KEY_FIELDS=("test_id","frame")
POS_FIELDS=("pos_x","pos_y","pos_z")
VEL_FIELDS=("vel_x","vel_y","vel_z")
INTEGER_FIELDS={
    "trace_version","test_id","frame","jump_pressed","jump_held","jump_released",
    "action_pressed","action_held","action_released","grounded","action_state",
    "surface_id","surface_flags","runtime_animation_id","action_slot","selector_provisional",
    "animation_requested_id","landing_classification_valid","landing_classified_request",
    "landing_impact_valid","landing_air_action","processed_analog_active",
    "processed_analog_valid","idle_counter",
    "circle_sample_valid","circle_raw_x","circle_raw_y",
    "circle_normalized_angle_bams",
    "sonic_act1_ran","animation_transition_state","animation_transition_source_id",
}
IGNORE_EXACT={"test_name"}
PC2004_SHA256="6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"

RETAIL_CORE_FIELDS={
    "test_id","frame","move_x","move_z","filtered_input_magnitude",
    "filtered_input_angle_bams","camera_yaw","pos_x","pos_y","pos_z",
    "vel_x","vel_y","vel_z","local_forward_speed","local_side_speed",
    "heading","grounded","action_state","animation_requested_id",
    "runtime_animation_id","animation_frame","sonic_act1_ran","tick_phase",
}

def validate_retail_metadata(path, reference_path):
    if not path:
        raise SystemExit("retail-trace authority requires --reference-metadata")
    with open(path,encoding="utf-8") as f: meta=json.load(f)
    required={
        "schema":"sa3ds-pc2004-retail-capture-v2",
        "authority":"RETAIL TRACE VERIFIED",
        "executable_sha256":PC2004_SHA256,
        "runtime_capture":True,
        "synthetic":False,
        "observational":True,
    }
    for key,value in required.items():
        if meta.get(key)!=value:
            raise SystemExit(f"invalid retail metadata {key}: {meta.get(key)!r}")
    h=hashlib.sha256()
    with open(reference_path,"rb") as f:
        for block in iter(lambda:f.read(1024*1024),b""): h.update(block)
    if meta.get("trace_sha256")!=h.hexdigest():
        raise SystemExit("retail metadata trace_sha256 does not match reference CSV")
    if not meta.get("scenario_id") or not meta.get("capture_utc"):
        raise SystemExit("retail metadata requires scenario_id and capture_utc")
    observed=set(meta.get("observed_fields",[]))
    if not RETAIL_CORE_FIELDS.issubset(observed):
        absent=sorted(RETAIL_CORE_FIELDS-observed)
        raise SystemExit("retail metadata missing core observed fields: "+",".join(absent))
    return meta

def load(path,frame_shift=0):
    with open(path,newline="",encoding="utf-8") as f:
        r=csv.DictReader(f)
        if not r.fieldnames: raise SystemExit(f"empty trace: {path}")
        rows={}
        for row in r:
            try: shifted=int(row.get("frame"))+frame_shift
            except (TypeError,ValueError): raise SystemExit(f"invalid frame in {path}: {row.get('frame')!r}")
            row=dict(row); row["frame"]=str(shifted)
            key=(row.get("test_id"),row.get("frame"))
            if key in rows: raise SystemExit(f"duplicate test/frame {key} in {path}")
            rows[key]=row
        return r.fieldnames,rows

def f3(row, fields):
    try: return tuple(float(row[x]) for x in fields)
    except (TypeError,ValueError,KeyError): return None

def d3(a,b):
    if a is None or b is None: return None
    return math.sqrt(sum((x-y)*(x-y) for x,y in zip(a,b)))

def close_field(field,a,b,abs_tol,rel_tol):
    if field in INTEGER_FIELDS or field in IGNORE_EXACT: return a==b
    try:
        x=float(a); y=float(b)
    except (TypeError,ValueError):
        return a==b
    return math.isclose(x,y,abs_tol=abs_tol,rel_tol=rel_tol)

def compare(ref_path,sa3ds_path,abs_tol=1e-6,rel_tol=1e-6,
            reference_kind="port-regression",observed_fields=None,
            reference_frame_shift=0,sa3ds_frame_shift=0):
    ref_fields,ref=load(ref_path,reference_frame_shift)
    sa_fields,sa=load(sa3ds_path,sa3ds_frame_shift)
    if observed_fields is not None:
        observed=set(observed_fields)
        common=[f for f in ref_fields if f in sa_fields and f in observed]
    else:
        common=[f for f in ref_fields if f in sa_fields]
    missing=[f for f in ref_fields if f not in sa_fields]
    extra=[f for f in sa_fields if f not in ref_fields]
    keys=sorted(set(ref)|set(sa),key=lambda k:(int(k[0] or -1),int(k[1] or -1)))
    mismatches=Counter(); first=None; exact=True; tolerance_match=True
    pos_err=[]; vel_err=[]
    for key in keys:
        rr=ref.get(key); ss=sa.get(key)
        if rr is None or ss is None:
            exact=False; tolerance_match=False; mismatches["missing_frame"]+=1
            if first is None: first=(key,"missing_frame",rr is not None,ss is not None)
            continue
        pe=d3(f3(rr,POS_FIELDS),f3(ss,POS_FIELDS)); ve=d3(f3(rr,VEL_FIELDS),f3(ss,VEL_FIELDS))
        if pe is not None: pos_err.append(pe)
        if ve is not None: vel_err.append(ve)
        for field in common:
            a=rr[field]; b=ss[field]
            if a!=b:
                exact=False; mismatches[field]+=1
                if first is None: first=(key,field,a,b)
                if not close_field(field,a,b,abs_tol,rel_tol): tolerance_match=False
    if missing or (extra and observed_fields is None):
        exact=False; tolerance_match=False
    if exact or tolerance_match:
        status={
            "retail-trace":"RETAIL TRACE VERIFIED",
            "source-equation":"SOURCE-EQUATION VERIFIED",
            "port-regression":"PORT-INTERNAL REGRESSION",
        }[reference_kind]
    else: status="NOT VERIFIED"
    return {
        "status":status,"exact":exact,"tolerance_match":tolerance_match,
        "first":first,"mismatches":mismatches,"missing":missing,"extra":extra,
        "pos_max":max(pos_err,default=0.0),"pos_mean":sum(pos_err)/len(pos_err) if pos_err else 0.0,
        "vel_max":max(vel_err,default=0.0),"vel_mean":sum(vel_err)/len(vel_err) if vel_err else 0.0,
        "frames":len(keys),
    }

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("reference"); ap.add_argument("sa3ds")
    ap.add_argument("--reference-kind",choices=("retail-trace","source-equation","port-regression"),
                    default="port-regression")
    ap.add_argument("--retail-live",action="store_true",
                    help="compatibility alias for --reference-kind retail-trace; metadata still required")
    ap.add_argument("--reference-metadata",
                    help="capture sidecar required before RETAIL TRACE VERIFIED can be emitted")
    ap.add_argument("--abs",dest="abs_tol",type=float,default=1e-6)
    ap.add_argument("--rel",dest="rel_tol",type=float,default=1e-6)
    ap.add_argument("--reference-frame-shift",type=int,default=0,
                    help="explicit ticks added to reference frame numbers before alignment")
    ap.add_argument("--sa3ds-frame-shift",type=int,default=0,
                    help="explicit ticks added to SA3DS frame numbers before alignment")
    args=ap.parse_args()
    kind="retail-trace" if args.retail_live else args.reference_kind
    meta=validate_retail_metadata(args.reference_metadata,args.reference) if kind=="retail-trace" else None
    r=compare(args.reference,args.sa3ds,args.abs_tol,args.rel_tol,kind,
              meta.get("observed_fields") if meta else None,
              args.reference_frame_shift,args.sa3ds_frame_shift)
    print(f"STATUS: {r['status']}")
    print(f"ALIGNED_KEYS: {r['frames']}")
    print(f"EXACT_MATCH: {'YES' if r['exact'] else 'NO'}")
    print(f"TOLERANCE_MATCH: {'YES' if r['tolerance_match'] else 'NO'}")
    if r['first']:
        key,field,a,b=r['first']
        print(f"FIRST_DIVERGENCE: test_id={key[0]} frame={key[1]} field={field} reference={a} sa3ds={b}")
    else:
        print("FIRST_DIVERGENCE: NONE")
    print(f"POSITION_ERROR_3D: max={r['pos_max']:.9g} mean={r['pos_mean']:.9g}")
    print(f"VELOCITY_ERROR_3D: max={r['vel_max']:.9g} mean={r['vel_mean']:.9g}")
    if r['mismatches']:
        print("FIELD_MISMATCH_COUNTS:")
        for field,count in sorted(r['mismatches'].items()): print(f"  {field}: {count}")
    if r['missing']: print("MISSING_SA3DS_FIELDS: "+",".join(r['missing']))
    if r['extra']: print("EXTRA_SA3DS_FIELDS: "+",".join(r['extra']))
    return 0 if r['status']!="NOT VERIFIED" else 1

if __name__=="__main__": raise SystemExit(main())
