import csv
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile

ROOT=pathlib.Path(__file__).resolve().parents[1]
HEADER=["trace_version","test_id","test_name","frame","move_x","move_z",
    "filtered_input_magnitude","filtered_input_angle_bams","camera_yaw",
    "pos_x","pos_y","pos_z","vel_x","vel_y","vel_z",
    "local_forward_speed","local_side_speed","heading","grounded","action_state",
    "animation_requested_id"]
LANDING_INTEGER_FIELDS=("animation_requested_id","landing_classification_valid",
    "landing_classified_request","landing_impact_valid","landing_air_action",
    "processed_analog_active","processed_analog_valid","idle_counter")

def write(path,dx=0.0):
    with open(path,"w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=HEADER); w.writeheader()
        for frame in range(3):
            w.writerow(dict(trace_version=1,test_id=3,test_name="TURN",frame=frame,
                move_x=0,move_z=1,filtered_input_magnitude=1,filtered_input_angle_bams=0,
                camera_yaw=0,pos_x=frame+dx,pos_y=1,pos_z=154+frame,
                vel_x=1,vel_y=0,vel_z=0,local_forward_speed=1,local_side_speed=0,
                heading=0,grounded=1,action_state=1,animation_requested_id=9))

def run(*args):
    return subprocess.run([sys.executable,str(ROOT/"tools/compare_white_zone_traces.py"),*map(str,args)],text=True,capture_output=True)

def main():
    with tempfile.TemporaryDirectory() as td:
        td=pathlib.Path(td); a=td/"reference.csv"; b=td/"sa3ds.csv"; c=td/"bad.csv"
        write(a); write(b); write(c,0.25)
        p=run(a,b)
        assert p.returncode==0 and "STATUS: PORT-INTERNAL REGRESSION" in p.stdout
        p=run(a,b,"--reference-kind","source-equation")
        assert p.returncode==0 and "STATUS: SOURCE-EQUATION VERIFIED" in p.stdout
        p=run(a,b,"--retail-live")
        assert p.returncode!=0 and "requires --reference-metadata" in p.stderr
        meta=td/"retail.json"
        meta.write_text(json.dumps({
            "schema":"sa3ds-pc2004-retail-capture-v1",
            "authority":"RETAIL TRACE VERIFIED",
            "executable_sha256":"6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74",
            "runtime_capture":True,"synthetic":False,"observational":True,
            "scenario_id":"neutral_standing","capture_utc":"2026-10-24T00:00:00Z",
            "trace_sha256":hashlib.sha256(a.read_bytes()).hexdigest(),
            "observed_fields":HEADER,
        }))
        p=run(a,b,"--retail-live","--reference-metadata",meta)
        assert p.returncode==0 and "STATUS: RETAIL TRACE VERIFIED" in p.stdout
        bad=json.loads(meta.read_text()); bad["synthetic"]=True
        meta.write_text(json.dumps(bad))
        assert run(a,b,"--retail-live","--reference-metadata",meta).returncode!=0
        p=run(a,c)
        assert p.returncode==1 and "STATUS: NOT VERIFIED" in p.stdout
        assert "FIRST_DIVERGENCE: test_id=3 frame=0 field=pos_x" in p.stdout
        assert "POSITION_ERROR_3D: max=0.25" in p.stdout
        # Tick alignment is explicit rather than inferred from unrelated starts.
        shifted=td/"shifted.csv"
        with shifted.open("w",newline="",encoding="utf-8") as f:
            w=csv.DictWriter(f,fieldnames=HEADER);w.writeheader()
            for row in csv.DictReader(a.open()): row["frame"]=str(int(row["frame"])+20);w.writerow(row)
        assert run(shifted,b).returncode!=0
        assert run(shifted,b,"--reference-frame-shift","-20").returncode==0
        # Appended v2 selector flags/counters must remain exact even when
        # near-equal numeric strings would pass the float tolerance.
        for field in LANDING_INTEGER_FIELDS:
            for path,value in ((a,"1"),(b,"1"),(c,"1.00000000001")):
                with open(path,"w",newline="",encoding="utf-8") as f:
                    fields=HEADER if field in HEADER else HEADER+[field]
                    w=csv.DictWriter(f,fieldnames=fields);w.writeheader()
                    row={k:"0" for k in HEADER}; row.update(trace_version=2,test_id=3,
                        test_name="LAND",frame=0,pos_y=1,**{field:value})
                    w.writerow(row)
            assert run(a,b).returncode==0
            p=run(a,c)
            assert p.returncode==1 and "STATUS: NOT VERIFIED" in p.stdout
            assert f"field={field}" in p.stdout
    print("White Zone trace comparator tests: PASS")

if __name__=="__main__": main()
