# Version 3.3.1 notes (improvement pass)

Surgical upgrade on top of v3.3 focused on the elevator system, render/GPU performance and runtime readiness.

## Changes

### Render / GPU optimisations
- **Box batching**: consecutive `drawBox` calls sharing the same colour are merged into one dynamic mesh draw, cutting CPU draw-call count from hundreds/thousands to a small number of batches.
- **Car fragment shader**:
  - `fbm` reduced from 4 → 2 octaves
  - Removed procedural tree-band noise from `skyEnv` (was extra noise + `atan`)
  - Replaced extreme `pow(x, 900/600/400)` with cheap polynomial approximations
  - Local-frame inverse uses `transpose` (yaw-only cars) instead of full `inverse`
- HUD bitmap text also benefits from colour batching.
- `renderer.endScene()` is called after the 3D pass so batches flush before the orthographic HUD.

### Elevator (new interactive system)
- Added `ElevatorSystem` class in `parking_world.h`.
- Cabin moves vertically between Ground (Y=0), B1 (-3.5 m) and B2 (-7.0 m).
- Door open / close animation with configurable durations.
- Automatic idle cycle + manual call keys:
  - `G` or `0` → call to Ground
  - `H` → call to B1
  - `J` → call to B2
- Visual cabin + split sliding doors drawn in `scene_view.h`.
- HUD line shows current floor and phase (IDLE / CLOSING / MOVING / OPENING / DWELL).
- All motion parameters are tagged `[ASSUMED]` in `aeon_layout.h`. This is an illustrative animation, **not** a survey of real AEON lifts.

### Layout constants
- New derived / assumed constants for cabin size, travel speed, door timings and level elevations.
- Existing geometry and traffic constants untouched.

### Application integration
- `Application` owns one shared `ElevatorSystem`.
- Updated every frame even when paused is false.
- `SceneRenderer::render` and `Hud::render` now receive the elevator reference.

### Motorbike status
- Still static parked models + B / X add-remove.
- Inventory management and capacity tests remain unchanged and green.
- Full behavioural AI (path following, parking manoeuvres) is the next recommended increment.

### B2 / geometry
- Existing B1↔B2 ramp and independent world inventories verified by the existing regression suite.
- No promotion of any `[ASSUMED]` value to real site data.

## Verification
- Headless world tests: **PASS** (direct g++ build + run).
- Existing traffic scenarios (5 cars, barriers, FULL, reverse park, column collision, B2 descent) remain covered.
- AddressSanitizer / UBSan were previously green on the base v3.3 package; the new elevator code is pure arithmetic + state machine and does not allocate dynamically beyond the single ElevatorSystem instance.

## Still outstanding / recommendations
1. Windows desktop build with real OpenGL + GLFW + GLEW + GLM (vcpkg recommended).
2. Motorbike movement AI (short path into free bay, leave after dwell).
3. Optional: elevator call buttons as 3D interactive objects.
4. Real B1/B2 floor plans if survey data becomes available – do **not** treat current numbers as measured.

## Build reminder (headless tests)
```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Itests/stubs \
    -o parking_world_tests tests/parking_world_tests.cpp
./parking_world_tests
```

## Desktop build (when graphics packages available)
```bash
cmake -S . -B build -DAEON_BUILD_APP=ON -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
