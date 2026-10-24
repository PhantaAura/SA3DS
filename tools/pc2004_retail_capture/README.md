# PC2004 retail controller trace capture

This folder is a portable, observational capture endpoint for the owner's exact
32-bit Windows copy of SADX PC2004. It contains no game code or data and never
patches the executable on disk. Live capture requires compatible 32-bit Windows
execution and the exact SHA-256:

`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`

Use 32-bit Python on Windows with `frida-tools`, start from an unmodified stage
restart, and capture one scenario per file:

```powershell
py -3-32 -m pip install frida-tools
py -3-32 capture_pc2004.py --exe "C:\path\to\sonic.exe" --scenario neutral_standing --output neutral.csv
```

The launcher verifies the executable before spawning it. `--pid PID` attaches
to an already running process instead. The agent reads the original filter at
`0x0040F170` and emits a row after Sonic's `0x00496F50` update. It does not
write game memory or call game functions. Stop after the requested test with
Ctrl+C, or use `--frames` for a bounded capture.

Initial protocol (restart the same Emerald Coast act, position and camera before
each capture): neutral standing; partial acceleration; full acceleration;
release/coast; reverse; running jump/landing; Spin Dash charge/release. Record
the controller source, stage position, camera setup and exact manual/macro input
timing with the resulting CSV. Do not compare traces until spawn, camera yaw,
coordinate mapping and update-tick alignment have been confirmed.

Each live CSV receives a `.csv.json` sidecar containing the executable identity,
capture authority, observed fields, timing/coordinate contract and trace digest.
The comparator refuses to emit `RETAIL TRACE VERIFIED` without a matching
non-synthetic sidecar. A local schema check is owner-data-free:

```sh
python3 capture_pc2004.py --self-test --output /tmp/pc2004_synthetic.csv
```

The self-test is explicitly `NOT VERIFIED`; it is never retail evidence.
