# Headless parking-world tests

Run from the project root:

```sh
cmake -S . -B build-tests -DAEON_BUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

The single CTest executable contains several deterministic scenarios:

1. Generate all 64 car stalls, the column grid and the 24 illustrative motorbike bays; fill and release motorbike capacity.
2. Create independent B1 and B2 worlds; check B1-to-B2 ramp elevation and drive a car down the ramp into a B2 stall.
3. Check the motorbike annex and elevator-core envelopes, shaft bounds and access dimensions.
4. Run five cars through entry/exit connectors, entry and exit barriers, payment queue and parking states while checking car/car and car/column overlap.
5. Sustain automatic arrivals until the 64-slot sample zone reaches `FULL`; check both aisles, both row sides, staging, reverse parking and rejection of another incoming car. This scenario repeats with three fixed random seeds.

The same test source was also compiled with AddressSanitizer and UndefinedBehaviorSanitizer; both passed in the development environment.

## Test-only GLM stub

`stubs/glm/glm.hpp` implements only the small `vec2`/`vec3` subset used by `parking_world.h` tests. It is **test-only** and is not a replacement for GLM. The OpenGL application must link to the actual GLM package. The CMake test target adds this directory privately to its include path only.

These tests validate the parking-world model and selected deterministic scenarios. They do not compile or exercise OpenGL rendering, prove deadlock-free traffic under every possible arrival pattern, or establish that assumed site geometry matches AEON's official drawings.
