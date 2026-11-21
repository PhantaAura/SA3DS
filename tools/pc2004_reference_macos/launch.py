#!/usr/bin/env python3
"""Prepare and launch an external owner's PC2004 reference in a Wine desktop."""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

PROFILE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
REPOSITORY = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--wine-app', type=Path, required=True)
    parser.add_argument('--owner-game', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True,
                        help='new external directory; private files stay here')
    parser.add_argument('--resolution', choices=['640x480', '800x600', '1024x768', '1152x864', '1280x1024'], default='640x480')
    parser.add_argument('--graphics-thread', choices=['single', 'auto'], default='single')
    parser.add_argument('--log-fps', action='store_true', help='log Wine present-rate estimates')
    parser.add_argument('--graphics-dir', type=Path,
                        help='optional external directory containing reference-only graphics DLLs')
    parser.add_argument('--fps-hud', action='store_true', help='request the DXVK FPS overlay')
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    source, output = args.owner_game.resolve(), args.output.resolve()
    if output == REPOSITORY or REPOSITORY in output.parents:
        parser.error('the private reference output must be outside the code tree')
    exe = source / 'sonic.exe'
    if hashlib.sha256(exe.read_bytes()).hexdigest() != PROFILE_SHA:
        parser.error('unsupported PC2004 executable fingerprint')
    contents = args.wine_app.resolve() / 'Contents'
    engine = contents / 'SharedSupport/wine/bin'
    for item in [engine / 'wine64', engine / 'wineserver',
                 contents / 'SharedSupport/prefix', source / 'system', source / 'SoundData']:
        if not item.exists():
            parser.error(f'missing reference input: {item}')
    graphics = args.graphics_dir.resolve() if args.graphics_dir else None
    graphics_names = ['d3d8.dll', 'd3d9.dll', 'dxvk.conf', 'libMoltenVK.dylib', 'MoltenVK_icd.json']
    if graphics and not (graphics / 'd3d8.dll').is_file():
        parser.error('graphics directory must contain d3d8.dll')
    # Refuse an existing destination, including symlinks. Never edit the owner's copy.
    output.mkdir(parents=True, exist_ok=False)
    game = output / 'game'
    game.mkdir()
    shutil.copy2(exe, game / 'sonic.exe')
    for name in ['system', 'SoundData']:
        (game / name).symlink_to(source / name, target_is_directory=True)
    if (source / 'SAVEDATA').is_dir():
        shutil.copytree(source / 'SAVEDATA', game / 'SAVEDATA')
    config = configparser.ConfigParser()
    config.optionxform = str
    config.read(source / 'sonicDX.ini')
    if not config.has_section('sonicDX'):
        config.add_section('sonicDX')
    config.set('sonicDX', 'screen', '0')
    presets = ['640x480', '800x600', '1024x768', '1152x864', '1280x1024']
    config.set('sonicDX', 'screensize', str(presets.index(args.resolution)))
    dimensions = [int(value) for value in args.resolution.split('x')]
    renderer = 'renderer=gl,csmt=0' if args.graphics_thread == 'single' else 'renderer=gl'
    with (game / 'sonicDX.ini').open('w', newline='\r\n') as stream:
        config.write(stream, space_around_delimiters=False)
    graphics_hashes = {}
    staged_graphics = output / 'graphics'
    if graphics:
        staged_graphics.mkdir()
        for name in graphics_names:
            item = graphics / name
            if not item.is_file():
                continue
            shutil.copy2(item, staged_graphics / name)
            graphics_hashes[name] = hashlib.sha256(item.read_bytes()).hexdigest()
            if name.endswith('.dll') or name == 'dxvk.conf':
                shutil.copy2(item, game / name)
    target = 'Z:' + str(game / 'sonic.exe').replace('/', '\\')
    command = [str(engine / 'wine64'), 'explorer', '/desktop=SADXOracle,' + args.resolution, target]
    (output / 'reference.json').write_text(json.dumps({
        'schema': 'sa3ds-pc2004-macos-reference-v1',
        'source_sha256': PROFILE_SHA, 'game': str(game),
        'renderer_requested': renderer, 'windowed': True, 'desktop': dimensions, 'command': command,
        'graphics_thread': args.graphics_thread, 'log_fps': args.log_fps,
        'graphics_sha256': graphics_hashes, 'fps_hud': args.fps_hud,
        'verification': 'prepared; gameplay comparison still required',
    }, indent=2) + '\n')
    print(f'Private reference prepared: {output}', flush=True)
    if args.prepare_only:
        return 0
    env = os.environ.copy()
    env.update(DYLD_FALLBACK_LIBRARY_PATH=str(contents / 'Frameworks'),
               WINEPREFIX=str(contents / 'SharedSupport/prefix'),
               WINESERVER=str(engine / 'wineserver'),
               WINE_D3D_CONFIG=renderer,
               WINEDEBUG='err+all,trace+fps' if args.log_fps else 'err+all', MVK_CONFIG_LOG_LEVEL='0')
    if graphics:
        overrides = [name[:-4] + '=n,b' for name in ['d3d8.dll', 'd3d9.dll'] if name in graphics_hashes]
        inherited = [item for item in env.get('WINEDLLOVERRIDES', '').split(';')
                     if item and not item.startswith(('d3d8=', 'd3d9='))]
        env['WINEDLLOVERRIDES'] = ';'.join(overrides + inherited)
        if 'libMoltenVK.dylib' in graphics_hashes:
            env['DYLD_LIBRARY_PATH'] = str(staged_graphics)
            env['MVK_CONFIG_FAST_MATH_ENABLED'] = '0'
        if 'MoltenVK_icd.json' in graphics_hashes:
            env['VK_ICD_FILENAMES'] = str(staged_graphics / 'MoltenVK_icd.json')
        if 'dxvk.conf' in graphics_hashes:
            env['DXVK_CONFIG_FILE'] = str(game / 'dxvk.conf')
        env['DXVK_LOG_PATH'] = str(output)
    if args.fps_hud:
        env['DXVK_HUD'] = 'fps'
    with (output / 'wine.log').open('wb') as log:
        result = subprocess.run(command, cwd=game, env=env, stdout=log, stderr=subprocess.STDOUT)
        if result.returncode:
            return result.returncode
        # Explorer may hand off before Sonic ends. Keep the session alive while
        # this isolated prefix has processes, rather than killing its game child.
        print('Explorer handed off; waiting for reference Wine processes.', flush=True)
        return subprocess.run([str(engine / 'wineserver'), '-w'], env=env).returncode


if __name__ == '__main__':
    sys.exit(main())
