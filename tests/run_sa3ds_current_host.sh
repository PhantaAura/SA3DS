#!/bin/sh
# Fast owner-data-free regression gate for cumulative White Zone development.
# Does NOT replace devkitARM compilation or real Old 3DS tests.
set -eu
cd "$(dirname "$0")/.."
export PYTHONDONTWRITEBYTECODE=1
printf '\n== inherited 0G White Zone deterministic host gate ==\n'
sh tests/run_0_0_9_0G_tests.sh
printf '\n== 1D CAM synthetic decode gate ==\n'
python3 tests/test_pc2004_cam.py
printf '\n== WIP1 visual source integration gate ==\n'
python3 tests/test_white_zone_visual_source.py
printf '\n== WIP2 GPU frame lifetime gate ==\n'
sh tests/run_white_zone_gpu_batch.sh
printf '\n== WIP3 animation presentation timeline gate ==\n'
sh tests/run_animation_tick_stability.sh
printf '\n== PC2004 SonicAnimData checked-in evidence gate ==\n'
python3 tests/test_pc2004_sonic_animdata_report.py
printf '\n== WIP4/WIP5 Sonic ACTION pose and all-key preflight ==\n'
sh tests/run_sonic_action_preflight.sh
printf '\n== WIP6 Sonic geometry GPU submission preflight ==\n'
sh tests/run_sonic_geometry_preflight.sh
printf '\n== WIP7 owner SA3C+SA3T visual and geometry CLI ==\n'
sh tests/run_sonic_owner_visual_cli.sh
printf '\n== WIP7 0G native visual reintegration ==\n'
python3 tests/test_0g_visual_reintegration.py
printf '\n== Run11 native owner-pair SD fallback gate ==\n'
sh tests/run_owner_pair_loader.sh
printf '\n== Run12 fixed 60Hz simulation cadence gate ==\n'
sh tests/run_white_zone_tick.sh
printf '\n== MacFix2: 0G native main.c lexical/HUD source gate ==\n'
python3 tests/test_0g_native_hud_lexical.py
printf '\n== BootTrace1 3-mode native source integration ==\n'
python3 tests/test_boottrace1_integration.py
printf '\n== TextureSyncFix Citro3D synchronization regression ==\n'
python3 tests/test_texture_sync_fix.py
printf '\n== CPUUpload2 software Morton tiling/source guards ==\n'
python3 tests/test_cpu_texture_tiling.py
printf '\n== Emerald Coast native visual-package reader ==\n'
sh tests/run_emerald_coast_visual_runtime.sh
sh tests/run_emerald_coast_gpu_runtime.sh
sh tests/run_emerald_coast_collision.sh
python3 tests/test_emerald_coast_native_integration.py
printf '\nSA3DS cumulative host (owner-data-free) gate: PASS\n'
printf '\n== Pass10A frozen-Pose Lab and geometry metrics ==\n'
sh tests/run_pass10a.sh

printf "\n== Native view/input/facing regression ==\n"
sh tests/run_player_view.sh

printf "\n== WCA bounded native replay protocol ==\n"
sh tests/run_wca_replay.sh

printf "\n== PC2004 deterministic half-turn tie ==\n"
sh tests/run_pc2004_half_turn.sh

printf "\n== Emerald Coast bounded collision regions ==\n"
sh tests/run_emerald_coast_regions.sh

printf "\n== Emerald Coast owner route metadata and residency ==\n"
sh tests/run_emerald_coast_route.sh
sh tests/run_emerald_coast_residency.sh
python3 tests/test_emerald_coast_full_integration.py

printf '\n== Direct ring runtime and importer ==\n'
bash tests/run_ring_runtime.sh
python3 tests/test_pc2004_ring_importer.py

printf '\n== Ring-group runtime ==\n'
bash tests/run_ring_groups.sh
python3 tests/test_pc2004_ring_groups_importer.py

printf '\n== Movement object definitions and spring command decoding ==\n'
bash tests/run_movement_objects.sh

printf '\n== Source spring launch preparation ==\n'
bash tests/run_spring_launch.sh

printf '\n== PC2004 source rotation quantization ==\n'
bash tests/run_pc2004_rotation_quantization.sh

printf '\n== PC2004 animated world hierarchy ==\n'
bash tests/run_sa3c_animated_world.sh

printf "\n== PC2004 grounded steering and force frame ==\n"
bash tests/run_pc2004_ground_steering.sh

printf "\n== EC capsule boundary precision ==\n"
bash tests/run_ec_edge_precision.sh

printf "\n== Player frame dispatch and flat ball steering ==\n"
bash tests/run_player_frame_dispatch.sh

printf "\n== Native texture override package and Dreamcast decoder ==\n"
bash tests/run_texture_override.sh

printf "\n== PC2004 ordinary ground clamp ==\n"
bash tests/run_pc2004_ground_accel.sh

printf "\n== Source spring actor and player command phases ==\n"
bash tests/run_spring_runtime.sh

printf "\n== Source spring player launch integration ==\n"
bash tests/run_spring_player.sh
