#!/usr/bin/env python3
import csv,json,pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
TOOL=ROOT/'tools/pc2004_retail_capture/capture_pc2004.py'
AGENT=(ROOT/'tools/pc2004_retail_capture/pc2004_trace_agent.js').read_text()
with tempfile.TemporaryDirectory() as td:
 out=pathlib.Path(td)/'synthetic.csv'
 p=subprocess.run([sys.executable,str(TOOL),'--self-test','--output',str(out)],text=True,capture_output=True)
 assert p.returncode==0,p.stderr
 rows=list(csv.DictReader(out.open())); assert len(rows)==3 and len(rows[0])==37
 assert [int(row['input_filter_sequence']) for row in rows]==[1,2,3]
 meta=json.loads(out.with_suffix('.csv.json').read_text())
 assert meta['authority']=='NOT VERIFIED' and meta['synthetic'] is True
 assert meta['runtime_capture'] is False and meta['observational'] is True
 assert '0x0040F170'.lower() in AGENT.lower() or 'RVA_INPUT_FILTER = 0x0000f170' in AGENT
 assert 'RVA_SONIC_ACT1 = 0x00096f50' in AGENT
 assert 'RVA_SONIC_MAIN = 0x0009a9b0' in AGENT
 assert "tick_phase:'post_sonic_main'" in AGENT
 assert 'vel_x:f32(m,0x04)' in AGENT and 'vel_z:f32(m,0x0c)' in AGENT
 assert 'transitionState === 2 ? transitionSource : requestedAnimation' in AGENT
 assert 'this.input = latestInput' in AGENT
 assert "stage:'sonic_main_input'" in AGENT
 for forbidden in ('writeFloat(','writeU32(','writeS32(','Memory.patchCode'):
  assert forbidden not in AGENT
source=TOOL.read_text()
assert 'if not ready:' in source and 'if errors:' in source
assert 'no Sonic Act1 row observed' in source
print('PC2004 retail capture endpoint: PASS (schema self-test; retail execution NOT VERIFIED)')
