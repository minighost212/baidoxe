# Headless parking-world tests

Build and run from the project root:

```sh
cmake -S . -B build-tests -DCMAKE_BUILD_TYPE=Release \
  -DAEON_BUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

`CMAKE_BUILD_TYPE=Release` is useful with single-configuration generators such as
Unix Makefiles; `--config Release` alone does not select an optimization mode for
those generators. Multi-configuration generators (for example Visual Studio)
select Release through `--config Release`.

The headless CTest executable covers the following model-level scenarios:

1. Generate the 64 car stalls, column grid and 24 illustrative motorbike bays; check motorbike capacity and release.
2. Create independent B1/B2 worlds, verify the B1-to-B2 ramp geometry, and drive a B2 car down to the B2 parking level.
3. Check the assumed motorbike-annex and elevator-core geometry envelopes.
4. Exercise motorbike aisle alignment, bay entry, parking, dwell, departure and bay release.
5. Exercise elevator door closing, vertical travel, door opening and arrival at Ground and B2.
6. Run five cars through entrance, parking, payment and exit; check car/car and car/column overlap and full clearing.
7. Exercise sustained arrivals across both aisles and both row sides, including staging, reverse parking and barrier flow, with fixed seeds.
8. Replay the two reported deadlock reproductions: five cars with seed `503`, and a 64-car fixed load with seed `500`.
9. Run the fixed-load regression matrix for `N = 5, 10, 16, 20, 24, 32, 40, 48, 64`, ten seeds per load (`500`–`509`), 1.5-second arrival spacing and `dt = 0.04`. Every frame checks approximate vehicle/column collisions, movement distance, slot ownership/orphan state and a 30-second watchdog. `Parked`, `EntryGateWait` and `ExitPaymentWait` are permitted waiting phases.
10. Start a deterministic FULL-lot fixture with 64 occupied stalls, reject a 65th arrival, then simulate until all 64 vehicles leave and all stalls are free. The fixture is intentionally seeded at the parked state to isolate and stress the full-lot departure path.
11. Repeat a 64-car scenario at `dt = 0.016, 0.04, 0.1, 0.25` and compare two complete 64-car replays with the same seed and `dt` for an identical frame-by-frame signature.
12. Run a five-car B2 lifecycle through arrival, parking and outbound ramp ascent, then verify the world and slot inventory are empty.

The supplied collision checks use axis-aligned rectangular footprints and are approximate; they are not a physics engine. These tests validate the world-model logic only. They do not prove deadlock-free operation for every possible traffic pattern, establish actual AEON site dimensions, or check OpenGL rendering, camera, lighting, model placement or Windows desktop runtime behavior. Layout values marked `[ASSUMED]` remain illustrative assumptions rather than measured AEON dimensions. The test-only GLM stub in `stubs/glm` is not used by the desktop application.
