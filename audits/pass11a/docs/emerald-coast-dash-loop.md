# Emerald Coast Act 1 dash panels and loop contact

The owner-data replay starts with a direct boost from an original SET placement and can then run nearby unconditional dash-panel tasks. It uses the native physics and static terrain adapters. It omits the rest of the SET task schedule and uses neutral controller input.

```sh
sh tools/run_host_dash_panel_path.sh \
  "$SA3DS_OWNER_ASSETS/emerald_coast_collision.sa3dspak" \
  "$SA3DS_OWNER_ASSETS/ec_movement_objects.sa3dspak" --chain
```

- Row 0 triggers row 12 on tick 6. Both source rows specify the default boost speed and a 100-tick control lock.
- In the chained replay Sonic loses support at tick 13 around (2204.24, 19.41, 256.11). The last support carries flags `0xc0000001`. Physics resets at tick 137.
- In the isolated row 0 replay the first miss occurs at tick 14. The projected contact is inside loop instance 426, triangle 10, at signed distance approximately −0.877. The point query rejects distances below −0.35.
- A local experiment allowing steep faces and finite edge contacts under an aligned loop frame reached the top near Y=89 and traversed the far side. It still detached at the exit near (2189.78, 24.23, 285.53) and reset. Those changes were discarded. A useful regression must cover the exit as well as the entrance.
- Owner SET row 193 is type 20 `WALL`, dispatched at `0x4D4850`. It initializes a CCL descriptor at `0x9BE818` and enqueues via `0x41C280`. The replay omits that object. Its effect on the approach has yet to be verified.
