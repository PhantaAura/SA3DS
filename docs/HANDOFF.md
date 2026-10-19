# SA3DS handoff

The working tree is the supplied Pass10A code-only project with Pass11A preserved in `audits/pass11a/` and a new independent Pass11B source-motion oracle.

Commands:

```sh
bash tests/run_sa3ds_current_host.sh

CHRMODELS_PATH="/Users/emeraldhunter/Library/Application Support/Steam/steamapps/common/Sonic Adventure DX/system/CHRMODELS_orig.dll" \
SONIC_PVM_PATH="/Users/emeraldhunter/Library/Application Support/Steam/steamapps/common/Sonic Adventure DX/system/SONIC.PVM" \
sh tests/run_pass11b_source_pose.sh

DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM sh tools/build_3ds.sh
```

The code-only output belongs in `dist/SA3DS_Pass11B_Audited.3dsx`. The 3DS still loads `sonic.sa3c` and `sonic.sa3t` from `sdmc:/3ds/SA3DS/assets/`; never add those files to this tree.

Latest verified result: Pass11A matches the owner's Pass10 metrics and source markers. Pass11B must remain green before any pose/import change is accepted. Real Old 3DS verification of the newly rebuilt artifact remains a user-side step.
