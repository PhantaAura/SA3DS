# SA3DS project rules

- The owner's exact 2004 PC SADX executable/data and `/Users/emeraldhunter/sonic.exe.c` are private reference material, not redistributable project source.
- Keep all Sega-derived converted assets outside this code tree. Generate them from the owner's installation into a temporary or SD destination.
- Pass10A is the runnable 3DS baseline; Pass11A and later audits constrain changes. Preserve the CPU texture-upload path, external SD owner-pair loader, and boot diagnostics unless real Old 3DS evidence supports replacing them.
- Do not claim retail parity, native build success, emulator success, or hardware success unless that exact check ran.
- Run `bash tests/run_sa3ds_current_host.sh` for owner-data-free changes. Run `tests/run_pass11b_source_pose.sh` with exact owner paths for pose/import changes. Build with `tools/build_3ds.sh` because this workspace path contains spaces.
- The next target is a faithful offline native reimplementation that consumes owner-generated packages; it is not a binary translation, DRM bypass, or redistributed game.

# Current workflow

- Work in the single `runtime/` tree; historical runtimes live in Git.
- Keep README.md, RESUME.md and PARITY_STATUS.json as the project entry points.
- Finish the normal-start Emerald Coast clear/results milestone before nonblocking polish.
- Label deviations CANDIDATE and retain the full fidelity goal after the playable milestone.
- Commit and push one finished behavior at a time, with its necessary evidence.
- Update the existing handoff/status instead of adding versioned checkpoint documents.
