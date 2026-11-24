# AEON MALL Đà Nẵng Thanh Khê – Parking Simulation (Version 3.3)

C++17 / OpenGL 3.3 / GLFW / GLEW / GLM. One unit in the code equals one metre.
The project models a representative assumed parking zone for AEON MALL Đà Nẵng Thanh Khê in the TTC Plaza podium. It is **not** an official or measured basement plan.

## 1. Version 3.3 changes

- Centralised parking-layout, vehicle envelope, ramp, barrier, camera, HUD, motorbike-annex and elevator-core dimensions in `aeon_layout.h` with `[SOURCE]`, `[DERIVED]` or `[ASSUMED]` labels.
- Generated car bays, column locations, traffic tracks, ramp geometry, motorbike bays and elevator geometry from the shared layout constants and helper functions.
- Added a 24-space illustrative motorbike annex on each simulated floor, with 8 static demo motorbikes initially parked. Press `B` to add and `X` to remove one.
- Added a two-shaft illustrative elevator/lift core and access walkway. This is a static architectural representation only; no lift car, door animation or vertical passenger simulation is implemented.
- Added a separate B2 simulation world at the assumed elevation `-7.0 m`. Its ramp starts at the assumed B1 elevation `-3.5 m` and descends to B2. B2 has its own independent 62-space sample inventory and demo motorbike annex; this does **not** mean the real site has 124 spaces in this arrangement.
- Set both barriers 3 m beyond the ramp foot, added a clear 2 m straight lead-in before the aisle turn, reduced the main aisles to 5.5 m, and removed the two stalls that blocked the entry/exit connector.
- Added CTest coverage for generated layout, motorbike capacity, B2 ramp/parking, elevator geometry bounds, multiple vehicles, route connectors, entry/exit barriers, payment queue, collision checks, two aisle directions, reverse parking and the `FULL` state.
- Made the traffic regression test deterministic by accepting an optional random seed in `ParkingWorld`.
- Fixed the last bitmap-font row index so glyph geometry derives from the shared glyph dimensions rather than a separate literal.

## 2. Project structure

| File | Purpose |
|---|---|
| `parking_main.cpp` | GLFW entry point, B1/B2 selection, frame loop and keyboard/mouse input |
| `aeon_layout.h` | Shared public facts, derived values, assumed layout/scene dimensions, camera/HUD settings and coordinate helper functions |
| `parking_world.h` | Car and motorbike demo inventories, traffic state machine, barrier timers, ramp routes and approximate collision checks |
| `scene_view.h` | 3D scene, AEON exterior massing, basement shell, ramp, barriers, motorcycle models, elevator core and HUD |
| `camera.h` | Orbit and top-down camera presets for exterior and basement views |
| `renderer.h` | OpenGL drawing primitives and procedural Vios renderer |
| `car_model.h` | Local mesh/profile control points for the Toyota Vios visual model; overall vehicle envelope is linked to `AeonLayout` dimensions |
| `vios.obj` | Original supplied reference asset retained in the project; current runtime model is generated procedurally by `car_model.h` |
| `tests/parking_world_tests.cpp` | Deterministic simulation and geometry regression tests |
| `tests/stubs/glm/glm.hpp` | Minimal GLM-compatible `vec2`/`vec3` test stub used only by the headless world tests; not used by the OpenGL app |
| `CMakeLists.txt` | Desktop app and headless CTest build options |
| `SURVEY_AEON.md` | Source provenance, derived values, assumed facilities and missing survey data |
| `PROJECT_AUDIT.md` | Requirement coverage and test status |
| `LAYOUT_SCHEMATIC.svg` | Plan-view schematic of the sample layout and auxiliary areas |

## 3. Modelled scope and data status

### Public source facts `[SOURCE]`

The supplied data describes the whole TTC Plaza complex: nearly 15,000 m² land area, more than 122,000 m² floor area, 18 floors above ground, two basement levels, a four-floor commercial podium, around 23,000 m² parking area, approximately 430 car spaces and 1,800 motorbike spaces. These are complex-wide figures, not per-floor or per-zone measurements.

### Derived values `[DERIVED]`

The 23,000 m² parking area divided by two basements gives an average 11,500 m² per basement for scale context only. The car-to-motorbike capacity ratio is approximately 1:4.2. Zone A's 62 sample car spaces are about 14.4% of the approximate whole-complex 430-car capacity.

### Assumed design `[ASSUMED]`

- Car parking core: 40 × 31 m; four rows of 16 perpendicular stalls, with two connector-clearance stalls omitted; 62 car spaces per simulated floor.
- Perimeter lanes: 6 m; representative Zone A car layout envelope: 52 × 43 m.
- Car stall: 2.5 × 5.0 m; main aisle: 5.5 m; column section: 0.6 × 0.6 m; column pitch: 7.5 m.
- Separate entry and exit lanes use their own barrier at `X=-23 m`; cars turn into the parking aisles at `X=-21 m`, 2 m after passing the barrier.
- B1 level: -3.5 m; B2 level: -7.0 m; clear height: 3.0 m.
- Ramp slope: 12%; paired lanes: 3.5 m each.
- The B2 ramp is a separate B1-to-B2 link using the same assumed 3.5 m level drop and 12% reference slope.
- Motorbike annex: two rows of 12 sample spaces (24 total per floor); each bay 0.85 × 2.0 m. This annex is outside the 52 × 43 m car-zone footprint and is not a measured AEON motorcycle area.
- Elevator core: two illustrative shafts, lobby and walkway. Its location and dimensions are assumed.
- 90–120 seconds dwell, 1.5-second automatic spawn interval, vehicle speeds, queue timing and barrier timing are demo settings, not real AEON operating data.

## 4. Controls

| Input | Action |
|---|---|
| `C` | Chuyển đổi tuần tự các góc nhìn hầm chân thực (Đi bộ FPS -> Buồng lái -> Bám đuôi -> Camera CCTV -> Sa bàn) |
| `K` hoặc `3` | Đổi kênh Camera an ninh giám sát CCTV (1: Cổng dốc vào, 2: Cổng ra, 3: Giao lộ hầm, 4: Sảnh thang máy, 5: Bãi xe máy) |
| `4` | Phím tắt góc nhìn người đi bộ (First-Person Pedestrian Walk) |
| `5` | Phím tắt góc nhìn buồng lái xe (Driver Cockpit View) |
| `6` | Phím tắt góc nhìn bám sau đuôi xe (Vehicle Chase View) |
| `7` | Phím tắt góc nhìn sa bàn toàn cảnh hầm (Basement Orbit View) |
| `V` | Chuyển đổi ngoại thất tòa nhà / tầng hầm B1 |
| `1` | Hiển thị tầng hầm B1 |
| `2` | Hiển thị tầng hầm B2 |
| `T` | Bật/tắt góc nhìn sơ đồ mặt bằng từ trên xuống (Top-Down 2D Map) |
| `U` | Bật / tắt hiển thị trần tầng hầm (Hệ dầm bê tông, ống PCCC đỏ, đèn LED, máng cáp) |
| Kéo chuột trái | Xoay góc nhìn tự do / Xoay sa bàn |
| Cuộn chuột | Phóng to / thu nhỏ (Zoom) |
| `W` / `S` | Tiến / lùi (Đi bộ trong hầm hoặc Lái xe thủ công) |
| `A` / `D` | Sang trái / phải (Đi bộ trong hầm hoặc Bẻ lái xe) |
| Phím mũi tên | Điều hướng đi bộ trong hầm |
| `Shift` (giữ) | Chạy nhanh khi đi bộ trong hầm |
| `P` | Tạm dừng / tiếp tục mô phỏng |
| `N` hoặc `SPACE` | Yêu cầu thêm 1 xe ô tô vào hầm |
| `F` | Bật / tắt xe tự động đến bãi |
| `M` | Bật / tắt chế độ lái xe thủ công (xe màu xanh dương) |
| `B` | Thêm 1 xe máy vào bãi xe máy tầng đang chọn |
| `X` | Bớt 1 xe máy |
| `ESC` | Thoát chương trình |

Both B1 and B2 worlds continue to update while the other is displayed. `F`, `N`, `M`, `B` and `X` affect the currently selected floor.

## 5. Build the desktop application on Windows

Requirements: Visual Studio 2022 with Desktop development with C++, CMake 3.16+, an OpenGL-capable driver, and development packages for GLFW3, GLEW and GLM. A typical vcpkg setup is:

```powershell
vcpkg install glfw3 glew glm
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
.\build\Release\aeon_parking.exe
```

Replace the vcpkg path with the path used on your machine. If the packages are already discoverable by CMake, omit `CMAKE_TOOLCHAIN_FILE`.

## 6. Run the headless simulation tests

The world-model tests can be built without OpenGL/GLFW/GLEW or a system GLM package. They use the small test-only vector stub in `tests/stubs`; this does not compile or test the renderer/window application.

```powershell
cmake -S . -B build-tests -DAEON_BUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

The tests check:

1. 62 generated car bays, expected column count and motorbike-bay capacity.
2. Separate B1/B2 world state, B1-to-B2 ramp elevation and a B2 car completing the parking path.
3. Motorbike-annex and elevator-shaft geometry bounds.
4. Five-car traffic, vehicle/column collision checks, entry barrier, exit barrier, payment queue and connector paths.
5. Both aisles and row sides, staging/reverse-parking phases, sustained automatic arrivals and rejection of new cars at `FULL`.

## 7. Important limitations

1. No official B1/B2 floor plan or on-site measurements were provided. All basement, gate, ramp, column, motorbike-annex and elevator-core dimensions/locations remain `[ASSUMED]` unless marked otherwise in `aeon_layout.h`.
2. Zone A's 62 car spaces are a representative sample. B1 and B2 each use an independent demo world with the same sample layout; this is a visualization choice and is not a claim about the site's real distribution of 430 spaces.
3. The motorbike models are static parked props. They can be added/removed for display, but motorbike arrival routes, movement, collision avoidance and dwell/exit logic are not implemented.
4. The elevator core is static geometry (two shafts, doors, lobby and access walkway). Elevator cars, opening/closing doors, vertical travel and passenger logic are not implemented.
5. The B2 route uses an assumed ramp from B1 to B2; exact real-world slope, alignment, gates and one-way/ two-way operation are unknown.
6. The parking payment wait is only an animation placeholder. No real fee calculation, QR/payment transaction or external device integration is implemented.
7. Car collision checks use approximate axis-aligned bounding footprints derived from the vehicle heading, with a vertical tolerance on ramps. The tests exercise selected repeatable cases; they do not prove deadlock-free operation for every possible traffic pattern or replace a physics/path-planning engine.
8. The regression tests cover the simulation model, not OpenGL rendering. A full Windows build and fresh runtime screenshots of exterior, B1 and B2 are still required to verify visual framing and graphics-driver compatibility.
9. `car_model.h` retains local Vios mesh-profile control points because they define the intrinsic vehicle shape. The overall car envelope, wheelbase, track and wheel radius are sourced from named `AeonLayout` constants; these mesh-local control points are not parking-layout or AEON facility coordinates.

For data provenance and measurements still required, see `SURVEY_AEON.md`. Every value marked `[ASSUMED]` must remain described as an assumption in the report/HUD, not as an AEON measurement.
