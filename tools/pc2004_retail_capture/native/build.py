#!/usr/bin/env python3
"""Build code-only Windows reference helpers and a host playback comparator."""
import argparse,hashlib,json,os,shlex,shutil,subprocess
from pathlib import Path

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--zig',type=Path,required=True);p.add_argument('--minhook-source',type=Path,required=True)
 p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 source=Path(__file__).resolve().parent;repo=source.parents[2];output=a.output.resolve();hook=a.minhook_source.resolve()
 if output==repo or repo in output.parents:p.error('build outside the source checkout')
 dependencies=[hook/'src'/name for name in ['buffer.c','hook.c','trampoline.c','hde/hde32.c']]
 for item in dependencies+[hook/'include/MinHook.h',hook/'LICENSE.txt']:
  if not item.is_file():p.error('missing MinHook input: '+str(item))
 output.mkdir(parents=True,exist_ok=False)
 env=os.environ.copy();env['ZIG_GLOBAL_CACHE_DIR']=str(output/'zig-global-cache');env['ZIG_LOCAL_CACHE_DIR']=str(output/'zig-local-cache')
 compiler=[str(a.zig.resolve()),'cc','-target','x86-windows-gnu','-msse2','-O2','-Wall','-Wextra','-Werror']
 subprocess.run(compiler+['-municode',str(source/'inject.c'),'-lbcrypt','-lpsapi','-o',str(output/'inject.exe')],env=env,check=True)
 subprocess.run(compiler+['-Wno-unused-parameter','-shared','-I'+str(hook/'include'),str(source/'observer.c')]+[str(x) for x in dependencies]+['-lbcrypt','-lpsapi','-o',str(output/'observer.dll')],env=env,check=True)
 host=shlex.split(os.environ.get('CC','cc'))
 subprocess.run(host+['-std=c11','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-I'+str(repo/'src/game'),'-I'+str(repo/'src/sa1/physics'),str(source/'check_playback.c'),str(repo/'src/game/sonic_animation_runtime.c'),str(repo/'src/sa1/physics/surface_frame.c'),str(repo/'src/sa1/physics/steam_player_parameters.c'),'-lm','-o',str(output/'check-playback')],check=True)
 loop_sources=['tools/pc2004_retail_capture/native/check_loop.c','src/game/automatic_loop.c',
  'src/sa1/physics/pc2004_loop.c','src/sa1/physics/pc2004_task_frame.c',
  'src/sa1/physics/surface_frame.c','src/sa1/physics/steam_player_parameters.c','src/sa3dspak/sa3dspak.c']
 subprocess.run(host+['-std=c11','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror',
  '-I'+str(repo/'src/game'),'-I'+str(repo/'src/sa1/physics'),'-I'+str(repo/'src/sa3dspak')]+
  [str(repo/item) for item in loop_sources]+['-lm','-o',str(output/'check-loop')],check=True)
 wall_sources=['tools/pc2004_retail_capture/native/check_walls.c','src/game/ec_ccube.c',
  'src/game/ccl_wall.c','src/game/owned_task_list.c','src/game/set_task_lifetime.c',
  'src/game/set_object_clip.c','src/game/set_preparation.c','src/sa3dspak/sa3dspak.c']
 subprocess.run(host+['-std=c11','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror',
  '-I'+str(repo/'src/game'),'-I'+str(repo/'src/sa1/physics'),'-I'+str(repo/'src/sa3dspak')]+
  [str(repo/item) for item in wall_sources]+['-lm','-o',str(output/'check-walls')],check=True)
 ground_sources=['tools/pc2004_retail_capture/native/check_ground.c','src/game/emerald_coast_collision.c',
  'src/game/pc2004_sphere_sweep.c','src/sa3dspak/sa3dspak.c','src/sa3c/sa3c_pose.c']
 ground_sources += [str(item.relative_to(repo)) for item in (repo/'src/sa1/physics').glob('*.c')]
 subprocess.run(host+['-std=c11','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror',
  '-I'+str(repo/'src/game'),'-I'+str(repo/'src/sa1/physics'),'-I'+str(repo/'src/sa3c'),'-I'+str(repo/'src/sa3dspak')]+
  [str(repo/item) for item in ground_sources]+['-lm','-o',str(output/'check-ground')],check=True)
 shutil.copy2(hook/'LICENSE.txt',output/'LICENSE.minhook.txt')
 manifest={'schema':'sa3ds-original-observer-build-v1','compiler_version':subprocess.check_output([str(a.zig.resolve()),'version'],text=True).strip(),
  'owner_material_included':False,'source_sha256':{item.name:digest(item) for item in source.iterdir() if item.suffix in ['.c','.h','.py']},
  'minhook_sha256':{str(item.relative_to(hook)):digest(item) for item in hook.rglob('*') if item.is_file() and (item.suffix in ['.c','.h'] or item.name=='LICENSE.txt')},
  'artifact_sha256':{name:digest(output/name) for name in ['inject.exe','observer.dll','check-playback','check-loop','check-walls','check-ground']}}
 (output/'build.json').write_text(json.dumps(manifest,indent=2)+'\n');print(output)
if __name__=='__main__':main()
