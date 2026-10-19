import csv
import pathlib
import subprocess
import sys
import tempfile

ROOT=pathlib.Path(__file__).resolve().parents[1]
HEADER=["trace_version","test_id","test_name","frame","pos_x","pos_y","pos_z","vel_x","vel_y","vel_z"]

def write(path,dx=0.0):
    with open(path,"w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=HEADER); w.writeheader()
        for frame in range(3):
            w.writerow(dict(trace_version=1,test_id=3,test_name="TURN",frame=frame,
                pos_x=frame+dx,pos_y=1,pos_z=154+frame,vel_x=1,vel_y=0,vel_z=0))

def run(*args):
    return subprocess.run([sys.executable,str(ROOT/"tools/compare_white_zone_traces.py"),*map(str,args)],text=True,capture_output=True)

def main():
    with tempfile.TemporaryDirectory() as td:
        td=pathlib.Path(td); a=td/"reference.csv"; b=td/"sa3ds.csv"; c=td/"bad.csv"
        write(a); write(b); write(c,0.25)
        p=run(a,b)
        assert p.returncode==0 and "STATUS: MATCHES MEASURED FIELDS" in p.stdout
        p=run(a,b,"--retail-live")
        assert p.returncode==0 and "STATUS: VERIFIED 1:1" in p.stdout
        p=run(a,c)
        assert p.returncode==1 and "STATUS: NOT VERIFIED" in p.stdout
        assert "FIRST_DIVERGENCE: test_id=3 frame=0 field=pos_x" in p.stdout
        assert "POSITION_ERROR_3D: max=0.25" in p.stdout
    print("White Zone trace comparator tests: PASS")

if __name__=="__main__": main()
