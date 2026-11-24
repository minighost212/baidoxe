#pragma once

// ============================================================================
// AEON MALL Da Nang Thanh Khe - representative basement Zone A layout.
// Coordinates: 1 unit = 1 metre; X = length, Z = width, Y = height.
// Positive source claims below are transcribed from the user-provided dataset.
// No dimension marked [ASSUMED] is a verified AEON site measurement.
// ============================================================================

#include <array>
#include <cmath>

namespace AeonLayout {

inline constexpr float PI = 3.14159265358979f; // [DERIVED] Mathematical constant used for rotations.
inline constexpr float GEOMETRY_HALF_RATIO = 0.5f; // [DERIVED] Shared half-dimension ratio used by generated geometry.
inline constexpr float WORLD_ORIGIN_X_M = 0.0f; // [DERIVED] Coordinate-system origin.
inline constexpr float WORLD_ORIGIN_Z_M = 0.0f; // [DERIVED] Coordinate-system origin.
inline constexpr float GROUND_LEVEL_Y_M = 0.0f; // [DERIVED] Reference grade before descending to B1.

// ------------------------------- Public project facts [SOURCE]
inline constexpr float COMPLEX_SITE_AREA_NEAR_M2 = 15000.0f; // [SOURCE] Nearly 15,000 m2, whole TTC Plaza complex.
inline constexpr float COMPLEX_TOTAL_FLOOR_AREA_MORE_THAN_M2 = 122000.0f; // [SOURCE] More than 122,000 m2, whole complex.
inline constexpr int ABOVE_GROUND_FLOORS = 18; // [SOURCE] TTC Plaza complex.
inline constexpr int BASEMENT_FLOORS = 2; // [SOURCE] TTC Plaza complex.
inline constexpr int COMMERCIAL_BASE_FLOORS = 4; // [SOURCE] Shared commercial podium.
inline constexpr float AEON_RETAIL_AREA_LOW_APPROX_M2 = 30000.0f; // [SOURCE] Lower end of reported 30,000-31,000 m2 range.
inline constexpr float AEON_RETAIL_AREA_HIGH_APPROX_M2 = 31000.0f; // [SOURCE] Upper end of reported 30,000-31,000 m2 range.
inline constexpr float AEON_GLA_APPROX_M2 = 21000.0f; // [SOURCE] Reported approximate GLA; other reports differ.
inline constexpr float COMPLEX_PARKING_AREA_NEAR_M2 = 23000.0f; // [SOURCE] Approximate parking area for the whole complex.
inline constexpr int COMPLEX_CAR_CAPACITY_APPROX = 430; // [SOURCE] Approximate capacity across both basement levels.
inline constexpr int COMPLEX_MOTORBIKE_CAPACITY_APPROX = 1800; // [SOURCE] Approximate capacity across both basement levels.
inline constexpr int CAR_FEE_FIRST_TWO_HOURS_VND = 25000; // [SOURCE] Published parking tariff, 2 first hours.
inline constexpr int CAR_FEE_EACH_ADDITIONAL_HOUR_VND = 10000; // [SOURCE] Published parking tariff, each additional hour.
inline constexpr int MOTORBIKE_FEE_FIRST_FOUR_HOURS_VND = 4000; // [SOURCE] Published parking tariff, first 4 hours.
inline constexpr int MOTORBIKE_FEE_EACH_ADDITIONAL_TWO_HOURS_VND = 2000; // [SOURCE] Published parking tariff, each additional 2 hours.
inline constexpr const char* COMPLEX_OPENING_DATE = "2026-07-03"; // [SOURCE] Opening date supplied in the dataset.
inline constexpr const char* PARKING_TARIFF_ANNOUNCED_DATE = "2026-06-29"; // [SOURCE] Announcement date supplied in the dataset.
inline constexpr const char* SITE_ADDRESS = "46 Dien Bien Phu, Thanh Khe, Da Nang"; // [SOURCE] Address supplied in the dataset.

// ------------------------------- Derived reference values [DERIVED]
inline constexpr float AVG_PARKING_AREA_PER_BASEMENT_M2 =
    COMPLEX_PARKING_AREA_NEAR_M2 / static_cast<float>(BASEMENT_FLOORS); // [DERIVED] 23,000 / 2 = 11,500 m2/floor.
inline constexpr float MOTORBIKES_PER_CAR_CAPACITY_RATIO =
    static_cast<float>(COMPLEX_MOTORBIKE_CAPACITY_APPROX) /
    static_cast<float>(COMPLEX_CAR_CAPACITY_APPROX); // [DERIVED] 1,800 / 430, approximately 4.2.
inline constexpr float CAR_AREA_PER_CAR_ESTIMATE_LOW_M2 = 25.0f; // [DERIVED] User-provided approximate range; assumes cars use most parking area.
inline constexpr float CAR_AREA_PER_CAR_ESTIMATE_HIGH_M2 = 30.0f; // [DERIVED] User-provided approximate range; includes circulation/other parking functions.

// -------------------------------- Vehicle reference [ASSUMED]
// Toyota Vios reference envelope from the user-provided design dataset.
inline constexpr float CAR_LENGTH_M = 4.41f; // [ASSUMED] Model reference; verify against the selected Vios generation.
inline constexpr float CAR_WIDTH_M = 1.74f; // [ASSUMED] Model reference; verify against the selected Vios generation.
inline constexpr float CAR_HEIGHT_M = 1.48f; // [ASSUMED] Model reference; verify against the selected Vios generation.
inline constexpr float CAR_WHEELBASE_M = 2.55f; // [ASSUMED] Model reference; verify against the selected Vios generation.
inline constexpr float CAR_TRACK_WIDTH_M = 1.474f; // [ASSUMED] Procedural mesh axle-track reference; not supplied as AEON data.
inline constexpr float CAR_WHEEL_RADIUS_M = 0.30f; // [ASSUMED] Procedural Vios mesh wheel radius.

// -------------------------------- Parking module [ASSUMED]
inline constexpr float STALL_W_M = 2.5f; // [ASSUMED] Reference car-space width; not surveyed at AEON.
inline constexpr float STALL_L_M = 5.0f; // [ASSUMED] Reference car-space length; not surveyed at AEON.
inline constexpr float STALL_ANGLE_DEG = 90.0f; // [ASSUMED] Perpendicular parking.
inline constexpr float AISLE_W_M = 6.0f; // [ASSUMED] Two-way aisle reference width.
inline constexpr float PERIMETER_LANE_W_M = 6.0f; // [ASSUMED] Perimeter circulation lane reference width.
inline constexpr float COLUMN_SIZE_M = 0.6f; // [ASSUMED] Square column section; not surveyed.
inline constexpr int STALLS_PER_COLUMN_SPAN = 3; // [ASSUMED] Column grid reference interval.
inline constexpr float COLUMN_SPACING_X_M = STALLS_PER_COLUMN_SPAN * STALL_W_M; // [DERIVED] 3 x 2.5 = 7.5 m.
inline constexpr float FLOOR_TO_CEILING_M = 3.0f; // [ASSUMED] Clear height reference.
inline constexpr float FLOOR_HEIGHT_B1_M = -3.5f; // [ASSUMED] Basement B1 level relative to ground level.
inline constexpr float FLOOR_HEIGHT_B2_M = -7.0f; // [ASSUMED] Basement B2 level relative to ground level.
inline constexpr float RAMP_VERTICAL_DROP_M =
    (GROUND_LEVEL_Y_M > FLOOR_HEIGHT_B1_M ? GROUND_LEVEL_Y_M - FLOOR_HEIGHT_B1_M
                                          : FLOOR_HEIGHT_B1_M - GROUND_LEVEL_Y_M); // [DERIVED] Absolute grade-to-B1 difference.
inline constexpr float RAMP_SLOPE = 0.12f; // [ASSUMED] 12% reference slope.
inline constexpr float RAMP_LANE_W_M = 3.5f; // [ASSUMED] Width of each separated ramp lane.
inline constexpr float BARRIER_ARM_LEN_M = 3.0f; // [ASSUMED] Reference barrier-arm length.

// Zone A geometry, exactly as specified in the user-provided dataset.
inline constexpr int MODULE_COUNT_Z = 2; // [ASSUMED] Two parallel 16 m modules.
inline constexpr int ROWS_PER_MODULE = 2; // [ASSUMED] Two parking rows around each module aisle.
inline constexpr float PARKING_CORE_LENGTH_X_M = 40.0f; // [ASSUMED] 16 x 2.5 m along X.
inline constexpr float MODULE_WIDTH_Z_M = STALL_L_M + AISLE_W_M + STALL_L_M; // [DERIVED] 16 m.
inline constexpr float PARKING_CORE_WIDTH_Z_M = MODULE_COUNT_Z * MODULE_WIDTH_Z_M; // [DERIVED] Two 16 m modules = 32 m.
inline constexpr int STALLS_PER_ROW = static_cast<int>(PARKING_CORE_LENGTH_X_M / STALL_W_M); // [DERIVED] 40 m / 2.5 m = 16 spaces per row.
inline constexpr int ROW_COUNT = MODULE_COUNT_Z * ROWS_PER_MODULE; // [DERIVED] 4 rows.
inline constexpr int TOTAL_ZONE_A_CAR_SLOTS = MODULE_COUNT_Z * ROWS_PER_MODULE * STALLS_PER_ROW; // [DERIVED] 64 stalls.
inline constexpr float PARKING_CORE_AREA_M2 = PARKING_CORE_LENGTH_X_M * PARKING_CORE_WIDTH_Z_M; // [DERIVED] 40 x 32 = 1,280 m2.
inline constexpr float ZONE_A_SHARE_OF_COMPLEX_CAR_CAPACITY_PERCENT =
    100.0f * static_cast<float>(TOTAL_ZONE_A_CAR_SLOTS) /
    static_cast<float>(COMPLEX_CAR_CAPACITY_APPROX); // [DERIVED] 64 / 430, approximately 14.9%.
inline constexpr float ZONE_LENGTH_X_M = PARKING_CORE_LENGTH_X_M + 2.0f * PERIMETER_LANE_W_M; // [DERIVED] 52 m.
inline constexpr float ZONE_WIDTH_Z_M = PARKING_CORE_WIDTH_Z_M + 2.0f * PERIMETER_LANE_W_M; // [DERIVED] 44 m.
inline constexpr float CORE_MIN_X_M = -PARKING_CORE_LENGTH_X_M * 0.5f; // [DERIVED] -20 m.
inline constexpr float CORE_MAX_X_M = PARKING_CORE_LENGTH_X_M * 0.5f; // [DERIVED] +20 m.
inline constexpr float CORE_MIN_Z_M = -PARKING_CORE_WIDTH_Z_M * 0.5f; // [DERIVED] -16 m.
inline constexpr float CORE_MAX_Z_M = PARKING_CORE_WIDTH_Z_M * 0.5f; // [DERIVED] +16 m.
inline constexpr float ZONE_MIN_X_M = -ZONE_LENGTH_X_M * 0.5f; // [DERIVED] -26 m.
inline constexpr float ZONE_MAX_X_M = ZONE_LENGTH_X_M * 0.5f; // [DERIVED] +26 m.
inline constexpr float ZONE_MIN_Z_M = -ZONE_WIDTH_Z_M * 0.5f; // [DERIVED] -22 m.
inline constexpr float ZONE_MAX_Z_M = ZONE_WIDTH_Z_M * 0.5f; // [DERIVED] +22 m.
inline constexpr std::array<float, MODULE_COUNT_Z> AISLE_CENTER_Z_M = {
    -PARKING_CORE_WIDTH_Z_M * 0.25f, PARKING_CORE_WIDTH_Z_M * 0.25f
}; // [DERIVED] Aisle centre positions at -8 m and +8 m.
inline constexpr float INBOUND_LANE_OFFSET_M = AISLE_W_M * 0.25f; // [DERIVED] Two directional tracks inside each 6 m aisle.
inline constexpr float ENTRY_LANE_CENTER_Z_M = -RAMP_LANE_W_M * 0.5f; // [DERIVED] Entry lane centre in the paired 7 m ramp.
inline constexpr float EXIT_LANE_CENTER_Z_M = RAMP_LANE_W_M * 0.5f; // [DERIVED] Exit lane centre in the paired 7 m ramp.
inline constexpr float RAMP_TOTAL_WIDTH_Z_M = 2.0f * RAMP_LANE_W_M; // [DERIVED] Two separated 3.5 m lanes.
inline constexpr float RAMP_LENGTH_X_M = RAMP_VERTICAL_DROP_M / RAMP_SLOPE; // [DERIVED] 3.5 / 0.12 = 29.1667 m.
inline constexpr float RAMP_START_X_M = ZONE_MIN_X_M - RAMP_LENGTH_X_M; // [DERIVED] Upper end of the ramp.
inline constexpr float RAMP_END_X_M = ZONE_MIN_X_M; // [DERIVED] Ramp meets B1 at the west edge of Zone A.
inline constexpr float RAMP_MID_X_M = (RAMP_START_X_M + RAMP_END_X_M) * 0.5f; // [DERIVED]
inline constexpr float RAMP_MID_Y_M = (GROUND_LEVEL_Y_M + FLOOR_HEIGHT_B1_M) * 0.5f; // [DERIVED] Midpoint between ground and B1.
inline const float RAMP_ANGLE_RAD = -std::atan(RAMP_SLOPE); // [DERIVED] Downward from west to east; runtime constant because C++17 std::atan is not constexpr.
inline constexpr float RAMP_GATE_APPROACH_OFFSET_M = 0.0f; // [ASSUMED] Gate is placed at the foot of the ramp entering B1.
inline constexpr float ENTRY_GATE_X_M = RAMP_END_X_M; // [DERIVED] Shared entry / exit checkpoint at the bottom of the ramp (meeting B1 slab).
inline constexpr float CONNECTOR_X_M = ZONE_MIN_X_M + PERIMETER_LANE_W_M * 0.5f; // [DERIVED] Centre of west perimeter lane.
inline constexpr int COLUMN_GRID_X_COUNT = static_cast<int>(PARKING_CORE_LENGTH_X_M / COLUMN_SPACING_X_M); // [DERIVED] Five full 7.5 m spans.
inline constexpr float COLUMN_GRID_REMAINDER_X_M = PARKING_CORE_LENGTH_X_M - COLUMN_GRID_X_COUNT * COLUMN_SPACING_X_M; // [DERIVED] 2.5 m final remainder.
inline constexpr float SCENE_CENTER_X_M = (RAMP_START_X_M + ZONE_MAX_X_M) * 0.5f; // [DERIVED] Camera framing centre.

// -------------------------------- AEON exterior massing [ASSUMED]
inline constexpr int MALL_TOWER_FLOORS = ABOVE_GROUND_FLOORS - COMMERCIAL_BASE_FLOORS; // [DERIVED] Four podium floors plus fourteen tower floors.
inline constexpr float MALL_PODIUM_FLOOR_HEIGHT_M = 3.6f; // [ASSUMED] Representative commercial-floor height.
inline constexpr float MALL_TOWER_FLOOR_HEIGHT_M = 2.8f; // [ASSUMED] Representative upper-floor height.
inline constexpr float MALL_PODIUM_HEIGHT_M = COMMERCIAL_BASE_FLOORS * MALL_PODIUM_FLOOR_HEIGHT_M; // [DERIVED]
inline constexpr float MALL_TOWER_HEIGHT_M = MALL_TOWER_FLOORS * MALL_TOWER_FLOOR_HEIGHT_M; // [DERIVED]
inline constexpr float MALL_TOTAL_HEIGHT_M = MALL_PODIUM_HEIGHT_M + MALL_TOWER_HEIGHT_M; // [DERIVED]
inline constexpr float MALL_PODIUM_LENGTH_X_M = 58.0f; // [ASSUMED] Podium massing beyond the representative basement footprint.
inline constexpr float MALL_PODIUM_WIDTH_Z_M = 50.0f; // [ASSUMED] Podium massing beyond the representative basement footprint.
inline constexpr int MALL_FACADE_PANEL_COUNT = 24; // [ASSUMED] Segments used to shape the flowing podium facade.
inline constexpr float MALL_FACADE_WAVE_AMPLITUDE_M = 0.85f; // [ASSUMED] Depth variation of the podium ribbons.
inline constexpr float MALL_FACADE_WAVE_FREQUENCY = 0.12f; // [ASSUMED] Wave frequency along the street frontage.
inline constexpr float MALL_FACADE_RIBBON_HEIGHT_M = 0.34f; // [ASSUMED] Horizontal facade-ribbon thickness.
inline constexpr float MALL_FACADE_RIBBON_DEPTH_M = 0.80f; // [ASSUMED] Horizontal facade-ribbon depth.
inline constexpr float MALL_FACADE_RIBBON_FRONT_OFFSET_M = 1.10f; // [ASSUMED] Ribbon stand-off from the podium face.
inline constexpr float MALL_PODIUM_WINDOW_HEIGHT_M = 2.35f; // [ASSUMED] Glazing height within each podium level.
inline constexpr int MALL_ARCH_COUNT = 3; // [ASSUMED] Large public-entry arches along the frontage.
inline constexpr std::array<float, MALL_ARCH_COUNT> MALL_ARCH_CENTER_X_M = {-18.0f, 0.0f, 18.0f}; // [ASSUMED]
inline constexpr float MALL_ARCH_RADIUS_M = 3.6f; // [ASSUMED] Public-entry arch span and rise.
inline constexpr float MALL_ARCH_SPRING_HEIGHT_M = 3.2f; // [ASSUMED] Height where each arch curve begins.
inline constexpr float MALL_ARCH_RING_THICKNESS_M = 0.62f; // [ASSUMED] Warm-colored arch surround thickness.
inline constexpr float MALL_ARCH_DEPTH_M = 0.90f; // [ASSUMED] Arch-frame depth.
inline constexpr int MALL_ARCH_SEGMENT_COUNT = 12; // [ASSUMED] Faceted segments used to approximate each arch.
inline constexpr float MALL_SIGN_WIDTH_M = 16.0f; // [ASSUMED] AEON frontage sign-board width.
inline constexpr float MALL_SIGN_HEIGHT_M = 2.8f; // [ASSUMED] AEON frontage sign-board height.
inline constexpr float MALL_SIGN_DEPTH_M = 0.42f; // [ASSUMED] AEON frontage sign-board depth.
inline constexpr float MALL_SIGN_BOTTOM_Y_M = 9.6f; // [ASSUMED] Sign placement on the commercial podium.
inline constexpr float MALL_TOWER_LEFT_CENTER_X_M = -13.0f; // [ASSUMED] Tower massing position.
inline constexpr float MALL_TOWER_RIGHT_CENTER_X_M = 12.0f; // [ASSUMED] Tower massing position.
inline constexpr float MALL_TOWER_CENTER_Z_M = 2.5f; // [ASSUMED] Towers set back from the main frontage.
inline constexpr float MALL_TOWER_LEFT_WIDTH_X_M = 18.0f; // [ASSUMED] Tower massing width.
inline constexpr float MALL_TOWER_RIGHT_WIDTH_X_M = 20.0f; // [ASSUMED] Tower massing width.
inline constexpr float MALL_TOWER_DEPTH_Z_M = 26.0f; // [ASSUMED] Tower massing depth.
inline constexpr int MALL_TOWER_WINDOW_BAYS = 5; // [ASSUMED] Repeated facade window bays per tower.
inline constexpr float MALL_TOWER_WINDOW_HEIGHT_M = 1.75f; // [ASSUMED] Repeated tower glazing height.
inline constexpr float MALL_TOWER_BALCONY_DEPTH_M = 0.58f; // [ASSUMED] Projected floor-edge / balcony band.
inline constexpr int MALL_STREET_TREE_COUNT = 7; // [ASSUMED] Decorative street trees along the frontage.
inline constexpr float MALL_STREET_TREE_SPACING_M = 8.5f; // [ASSUMED] Street-tree spacing.
inline constexpr float MALL_STREET_CENTER_Z_M = -36.0f; // [ASSUMED] Street foreground position.
inline constexpr float MALL_STREET_WIDTH_Z_M = 12.0f; // [ASSUMED] Street foreground depth.
inline constexpr float MALL_STREET_LENGTH_X_M = 72.0f; // [ASSUMED] Street length across the mall frontage.
inline constexpr float MALL_STREET_TREE_LINE_Z_M = -29.5f; // [ASSUMED] Tree line between sidewalk and road.
inline constexpr float MALL_SIDEWALK_WIDTH_Z_M = 1.8f; // [ASSUMED] Frontage sidewalk width.
inline constexpr int MALL_STREET_DASH_COUNT = 12; // [ASSUMED] Foreground road-lane dashes.
inline constexpr float MALL_STREET_DASH_LENGTH_M = 2.8f; // [ASSUMED] Foreground road marking length.
inline constexpr float MALL_STREET_DASH_SPACING_M = 5.8f; // [ASSUMED] Foreground road marking spacing.
inline constexpr float MALL_STREET_MARKING_WIDTH_M = 0.14f; // [ASSUMED] Foreground road marking width.
inline constexpr float MALL_TOWER_WINDOW_BASE_OFFSET_M = 0.55f; // [ASSUMED] Tower window sill above each floor.
inline constexpr float MALL_TOWER_MULLION_WIDTH_M = 0.12f; // [ASSUMED] Repeated tower-window divider width.
inline constexpr float MALL_TOWER_FLOOR_BAND_HEIGHT_M = 0.24f; // [ASSUMED] Projected tower floor-edge band height.
inline constexpr float MALL_ARCH_GLASS_HEIGHT_M = 6.6f; // [ASSUMED] Dark glazing behind the entry arches.
inline constexpr float MALL_SIGN_TEXT_PIXEL_SIZE_M = 0.22f; // [ASSUMED] World-space bitmap lettering size.
inline constexpr float MALL_STREET_TREE_TRUNK_HEIGHT_M = 2.5f; // [ASSUMED] Decorative tree-trunk height.
inline constexpr float MALL_STREET_TREE_TRUNK_WIDTH_M = 0.34f; // [ASSUMED] Decorative tree-trunk width.
inline constexpr float MALL_STREET_TREE_CANOPY_WIDTH_M = 2.8f; // [ASSUMED] Low-poly canopy width.
inline constexpr float MALL_STREET_TREE_CANOPY_HEIGHT_M = 2.0f; // [ASSUMED] Low-poly canopy height.
inline constexpr float MALL_STREET_TREE_CANOPY_DEPTH_M = 2.4f; // [ASSUMED] Low-poly canopy depth.
inline constexpr float MALL_CAMERA_TARGET_X_M = WORLD_ORIGIN_X_M; // [DERIVED] Keep exterior framing centered on the mall.
inline constexpr float MALL_CAMERA_TARGET_Y_M = MALL_TOTAL_HEIGHT_M * 0.42f; // [DERIVED] Bias framing toward the public facade.

// Scale factor applied to the imported aeon_danang_v3.obj model. The OBJ
// bounding box spans ~0.95 m in X and ~1.80 m in Z; multiplying by 30 seats
// the building inside the existing podium/camera framing (~54 m wide).
inline constexpr float MALL_OBJ_SCALE_M = 30.0f; // [ASSUMED] Tune if the imported building is too large or small.
inline constexpr float MALL_OBJ_Y_LIFT_M = 8.0f; // [ASSUMED] Lift the imported building so its podium base sits above ground (the OBJ walls extend below the podium).
inline constexpr float MALL_OBJ_FRONT_Z_M = -27.0f; // [DERIVED] Front edge after centering and scaling the OBJ.

// -------------------------------- Physical detail / rendering [ASSUMED]
inline constexpr float FLOOR_SLAB_THICKNESS_M = 0.25f; // [ASSUMED] Visual shell thickness only.
inline constexpr float WALL_THICKNESS_M = 0.35f; // [ASSUMED] Visual wall thickness only.
inline constexpr float COLUMN_SAFETY_MARGIN_M = 0.0f; // [ASSUMED] No extra clearance beyond the column footprint.
inline constexpr float SLOT_COLUMN_CLEARANCE_OFFSET_M = 0.15f; // [ASSUMED] Small bay-centre offset beside every 3-stall column grid.
inline constexpr float BAY_TURN_STAGING_OFFSET_M = 0.5f; // [ASSUMED] Lateral staging offset before the 90-degree bay maneuver.
inline constexpr float VEHICLE_SAFETY_GAP_M = 0.35f; // [ASSUMED] Simulation-only separation, not an AEON operating rule.
inline constexpr float BAY_MARKING_WIDTH_M = 0.05f; // [ASSUMED] Visual marking thickness.
inline constexpr float MARKING_HEIGHT_M = 0.025f; // [ASSUMED] Visual marking thickness above floor.
inline constexpr float ARROW_LENGTH_M = STALL_W_M; // [DERIVED] Arrow graphic scales from stall width.
inline constexpr float ARROW_STROKE_M = 0.12f; // [ASSUMED] Visual arrow stroke width.
inline constexpr float BARRIER_POST_SIZE_M = 0.22f; // [ASSUMED] Simplified prop size.
inline constexpr float BARRIER_POST_HEIGHT_M = 1.0f; // [ASSUMED] Simplified prop height.
inline constexpr float BARRIER_ARM_THICKNESS_M = 0.12f; // [ASSUMED] Simplified arm thickness.
inline constexpr float BARRIER_PIVOT_HEIGHT_M = 0.85f; // [ASSUMED] Simplified arm pivot height.
inline constexpr float RAMP_SLAB_THICKNESS_M = 0.20f; // [ASSUMED] Visual ramp slab thickness.
inline constexpr float RAMP_EDGE_WALL_HEIGHT_M = 0.50f; // [ASSUMED] Visual ramp curb height.
inline constexpr float RAMP_EDGE_WALL_THICKNESS_M = 0.20f; // [ASSUMED] Visual ramp curb thickness.
inline constexpr float RAMP_RETAINING_WALL_HEIGHT_M = 3.0f; // [ASSUMED] Concrete side-wall height above the ramp.
inline constexpr float RAMP_RETAINING_WALL_THICKNESS_M = 0.40f; // [ASSUMED] Visual retaining-wall thickness.
inline constexpr float RAMP_SAFETY_STRIPE_WIDTH_M = 0.44f; // [ASSUMED] Width of each diagonal yellow wall stripe.
inline constexpr float RAMP_SAFETY_STRIPE_HEIGHT_M = 1.20f; // [ASSUMED] Height of wall-mounted hazard band.
inline constexpr float RAMP_SAFETY_STRIPE_SPACING_M = 1.0f; // [ASSUMED] Repeating hazard-band spacing.
inline constexpr float RAMP_SAFETY_STRIPE_ANGLE_RAD = 0.45f; // [ASSUMED] Diagonal angle of the wall hazard bands.
inline constexpr float ENTRANCE_PORTAL_CLEAR_WIDTH_M = 7.80f; // [ASSUMED] Clear width for the paired ramp lanes.
inline constexpr float ENTRANCE_PORTAL_CLEAR_HEIGHT_M = 3.40f; // [ASSUMED] Vehicle clearance beneath entrance canopy.
inline constexpr float ENTRANCE_PORTAL_PIER_WIDTH_M = 0.90f; // [ASSUMED] Concrete portal-pier width.
inline constexpr float ENTRANCE_PORTAL_DEPTH_M = 1.10f; // [ASSUMED] Concrete portal depth along the ramp.
inline constexpr float ENTRANCE_PORTAL_HEADER_HEIGHT_M = 0.65f; // [ASSUMED] Concrete portal-header thickness.
inline constexpr float RAMP_LANE_MARKING_WIDTH_M = 0.12f; // [ASSUMED] Painted lane-marking width.
inline constexpr float RAMP_LANE_DASH_LENGTH_M = 2.0f; // [ASSUMED] Painted directional dash length.
inline constexpr float RAMP_APRON_LENGTH_M = 2.8f; // [ASSUMED] Flat street-side transition slab length.
inline constexpr float RAMP_APRON_EXTRA_WIDTH_M = 0.8f; // [ASSUMED] Apron overhang beyond both ramp edges.
inline constexpr int RAMP_LANE_DASH_COUNT = 7; // [ASSUMED] Number of visible dashes in each ramp lane.
inline constexpr float RAMP_LANE_DASH_START_OFFSET_M = 3.7f; // [ASSUMED] First dash offset from the ramp mouth.
inline constexpr float RAMP_LANE_DASH_SPACING_M = 3.6f; // [ASSUMED] Repeating spacing between ramp dashes.
inline constexpr float RAMP_EDGE_MARKING_OFFSET_M = 1.45f; // [ASSUMED] Lane-edge paint offset from each lane centre.
inline constexpr float RAMP_STRIPE_START_OFFSET_M = 1.2f; // [ASSUMED] First wall hazard stripe offset from the ramp mouth.
inline constexpr float RAMP_STRIPE_END_MARGIN_M = 0.6f; // [ASSUMED] Clear space before the B1 ramp foot.
inline constexpr float RAMP_STRIPE_FACE_OFFSET_M = 0.018f; // [ASSUMED] Stripe offset from the retaining-wall face.
inline constexpr float RAMP_STRIPE_DEPTH_M = 0.035f; // [ASSUMED] Thin wall-panel depth.
inline constexpr float ENTRANCE_PORTAL_START_OFFSET_M = 0.35f; // [ASSUMED] Portal position from the ramp mouth.
inline constexpr float ENTRANCE_FASCIA_FACE_GAP_M = 0.035f; // [ASSUMED] Small stand-off from the portal face.
inline constexpr float ENTRANCE_FASCIA_THICKNESS_M = 0.06f; // [ASSUMED] AEON-colour fascia depth.
inline constexpr float ENTRANCE_FASCIA_HEIGHT_M = 0.34f; // [ASSUMED] AEON-colour fascia height.
inline constexpr float ENTRANCE_FASCIA_WIDTH_M = 3.8f; // [ASSUMED] AEON-colour fascia width.
inline constexpr int GATE_SENSOR_STRIP_COUNT = 2; // [ASSUMED] Inductive-loop visual strips per lane.
inline constexpr float GATE_SENSOR_START_OFFSET_M = 1.35f; // [ASSUMED] First sensor distance before the barrier.
inline constexpr float GATE_SENSOR_SPACING_M = 0.55f; // [ASSUMED] Sensor-strip spacing.
inline constexpr float GATE_SENSOR_LENGTH_M = 0.08f; // [ASSUMED] Sensor-strip length along the ramp.
inline constexpr float GATE_SENSOR_HEIGHT_M = 0.018f; // [ASSUMED] Sensor-strip thickness.
inline constexpr float GATE_SENSOR_EDGE_MARGIN_M = 0.35f; // [ASSUMED] Clearance to ramp-lane edges.
inline constexpr float GATE_BOOTH_LONGITUDINAL_OFFSET_M = 0.75f; // [ASSUMED] Booth offset before the barrier.
inline constexpr float GATE_BOOTH_SIDE_MARGIN_M = 0.45f; // [ASSUMED] Booth setback from the lane edge.
inline constexpr float GATE_BOOTH_WIDTH_M = 0.72f; // [ASSUMED] Reader-booth width along the ramp.
inline constexpr float GATE_BOOTH_HEIGHT_M = 1.25f; // [ASSUMED] Reader-booth height.
inline constexpr float GATE_BOOTH_DEPTH_M = 0.60f; // [ASSUMED] Reader-booth depth across the lane.
inline constexpr float GATE_BOOTH_CAP_WIDTH_M = 0.78f; // [ASSUMED] Reader-booth top-cap width.
inline constexpr float GATE_BOOTH_CAP_HEIGHT_M = 0.10f; // [ASSUMED] Reader-booth top-cap thickness.
inline constexpr float GATE_BOOTH_CAP_DEPTH_M = 0.66f; // [ASSUMED] Reader-booth top-cap depth.
inline constexpr float GATE_READER_X_OFFSET_M = 0.08f; // [ASSUMED] Reader panel offset along the ramp.
inline constexpr float GATE_READER_Y_OFFSET_M = 0.58f; // [ASSUMED] Reader panel height above the ramp.
inline constexpr float GATE_READER_WIDTH_M = 0.27f; // [ASSUMED] Reader panel width.
inline constexpr float GATE_READER_HEIGHT_M = 0.36f; // [ASSUMED] Reader panel height.
inline constexpr float GATE_READER_DEPTH_M = 0.035f; // [ASSUMED] Reader panel depth.
inline constexpr float GATE_STATUS_Y_OFFSET_M = 1.03f; // [ASSUMED] Status-light height above the ramp.
inline constexpr float GATE_STATUS_WIDTH_M = 0.19f; // [ASSUMED] Status-light width.
inline constexpr float GATE_STATUS_HEIGHT_M = 0.14f; // [ASSUMED] Status-light height.
inline constexpr float GATE_STATUS_DEPTH_M = 0.045f; // [ASSUMED] Status-light depth.
inline constexpr int BARRIER_ARM_STRIPE_COUNT = 8; // [ASSUMED] Alternating reflective bands on each arm.
inline constexpr float LIGHT_FIXTURE_LENGTH_M = 1.8f; // [ASSUMED] Decorative ceiling fixture length.
inline constexpr float LIGHT_FIXTURE_WIDTH_M = 0.45f; // [ASSUMED] Decorative light fixture width.
inline constexpr float LIGHT_FIXTURE_THICKNESS_M = 0.08f; // [ASSUMED] Decorative light fixture thickness.


// ------------------------------- Additional representative facilities [ASSUMED]
// Auxiliary facilities are drawn beside (not inside) the 52 x 44 m Zone A core.
// They are demonstration geometry only; AEON's detailed locations/counts were not public.
inline constexpr float BIKE_STALL_W_M = 0.85f; // [ASSUMED] Demonstration motorbike-space width.
inline constexpr float BIKE_STALL_L_M = 2.0f; // [ASSUMED] Demonstration motorbike-space length.
inline constexpr int BIKE_STALLS_PER_ROW = 12; // [ASSUMED] Demonstration row count.
inline constexpr int BIKE_ROW_COUNT = 2; // [ASSUMED] Two small sample rows.
inline constexpr int TOTAL_ZONE_A_BIKE_SLOTS = BIKE_STALLS_PER_ROW * BIKE_ROW_COUNT; // [DERIVED] 24 sample spaces.
inline constexpr int INITIAL_PARKED_MOTORBIKES = 8; // [ASSUMED] Visible demo population, not site capacity.
inline constexpr float BIKE_ROW_AISLE_W_M = 0.60f; // [ASSUMED] Separation between demonstration rows.
inline constexpr float BIKE_ZONE_MARGIN_M = 0.20f; // [ASSUMED] Painted-zone edge margin.
inline constexpr float BIKE_ZONE_WIDTH_X_M =
    BIKE_STALLS_PER_ROW * BIKE_STALL_W_M + 2.0f * BIKE_ZONE_MARGIN_M; // [DERIVED]
inline constexpr float BIKE_ZONE_DEPTH_Z_M =
    2.0f * BIKE_STALL_L_M + BIKE_ROW_AISLE_W_M + 2.0f * BIKE_ZONE_MARGIN_M; // [DERIVED]
inline constexpr float BIKE_ZONE_GAP_FROM_ZONE_M = 0.80f; // [ASSUMED] Gap from car Zone A perimeter.
inline constexpr float BIKE_ZONE_CENTER_X_M = WORLD_ORIGIN_X_M; // [DERIVED] Centre aligned to the main layout.
inline constexpr float BIKE_ZONE_CENTER_Z_M =
    ZONE_MAX_Z_M + BIKE_ZONE_GAP_FROM_ZONE_M + BIKE_ZONE_DEPTH_Z_M * 0.5f; // [DERIVED]
inline constexpr float BIKE_BODY_LENGTH_M = 1.45f; // [ASSUMED] Simplified visual body envelope.
inline constexpr float BIKE_BODY_WIDTH_M = 0.34f; // [ASSUMED] Simplified visual body envelope.
inline constexpr float BIKE_BODY_HEIGHT_M = 0.48f; // [ASSUMED] Simplified visual body envelope.
inline constexpr float BIKE_WHEEL_RADIUS_M = 0.28f; // [ASSUMED] Simplified visual wheel radius.
inline constexpr float BIKE_WHEEL_WIDTH_M = 0.12f; // [ASSUMED] Simplified visual wheel width.
inline constexpr float BIKE_HANDLEBAR_WIDTH_M = 0.72f; // [ASSUMED] Simplified visual handlebar width.
inline constexpr float BIKE_SEAT_LENGTH_M = 0.62f; // [ASSUMED] Simplified visual seat length.
inline constexpr float BIKE_STALL_LINE_WIDTH_M = 0.035f; // [ASSUMED] Painted marking width.
inline constexpr float BIKE_WHEEL_OFFSET_RATIO = 0.34f; // [ASSUMED] Wheel position along the bike length.
inline constexpr float BIKE_BODY_LENGTH_RATIO = 0.58f; // [ASSUMED] Simplified body proportion.
inline constexpr float BIKE_BODY_RAISE_RATIO = 1.65f; // [ASSUMED] Body centre height relative to wheel radius.
inline constexpr float BIKE_SEAT_RAISE_RATIO = 2.15f; // [ASSUMED] Seat height relative to wheel radius.
inline constexpr float BIKE_HANDLEBAR_RAISE_RATIO = 2.25f; // [ASSUMED] Handlebar height relative to wheel radius.
inline constexpr float BIKE_SEAT_WIDTH_RATIO = 1.05f; // [ASSUMED] Seat width proportion.
inline constexpr float BIKE_SEAT_HEIGHT_RATIO = 0.18f; // [ASSUMED] Seat thickness proportion.
inline constexpr float BIKE_HANDLEBAR_Z_RATIO = 0.43f; // [ASSUMED] Handlebar position.
inline constexpr float BIKE_FRONT_LAMP_Z_RATIO = 0.30f; // [ASSUMED] Front lamp position.
inline constexpr float BIKE_FRONT_LAMP_WIDTH_RATIO = 0.70f; // [ASSUMED] Front lamp width proportion.
inline constexpr float BIKE_FRONT_LAMP_HEIGHT_RATIO = 0.35f; // [ASSUMED] Front lamp height proportion.
inline constexpr float BIKE_FRONT_LAMP_LENGTH_RATIO = 0.25f; // [ASSUMED] Front lamp length proportion.
inline constexpr float ELEVATOR_SHAFT_X_OFFSET_RATIO = 0.11f; // [ASSUMED] Lift shaft placement within the core.
inline constexpr float ELEVATOR_WALKWAY_WIDTH_M = 3.4f; // [ASSUMED] Access walkway serves two illustrative lift doors.
inline constexpr float ELEVATOR_WALKWAY_HEIGHT_M = 0.12f; // [ASSUMED] Access walkway slab thickness.
inline constexpr float ELEVATOR_OPENING_MARGIN_M = 0.30f; // [ASSUMED] Clearance beyond the lift walkway at the east wall.
inline constexpr float ELEVATOR_OPENING_WIDTH_Z_M =
    ELEVATOR_WALKWAY_WIDTH_M + 2.0f * ELEVATOR_OPENING_MARGIN_M; // [DERIVED]
inline constexpr float BIKE_ZONE_ACCESS_OPENING_MARGIN_M = 0.50f; // [ASSUMED] Clearance around the motorbike access.
inline constexpr float BIKE_ZONE_ACCESS_OPENING_WIDTH_X_M =
    BIKE_ZONE_WIDTH_X_M + 2.0f * BIKE_ZONE_ACCESS_OPENING_MARGIN_M; // [DERIVED]
inline constexpr float BIKE_ZONE_ACCESS_PATH_THICKNESS_M = 0.08f; // [ASSUMED] Flat link between the zone and bike annex.

inline constexpr float ELEVATOR_CORE_WIDTH_X_M = 5.6f; // [ASSUMED] Demonstration lift-core width.
inline constexpr float ELEVATOR_CORE_DEPTH_Z_M = 4.8f; // [ASSUMED] Demonstration lift-core depth.
inline constexpr float ELEVATOR_CORE_GAP_M = 1.2f; // [ASSUMED] Clearance outside the east perimeter wall.
inline constexpr float ELEVATOR_CORE_CENTER_X_M =
    ZONE_MAX_X_M + ELEVATOR_CORE_GAP_M + ELEVATOR_CORE_WIDTH_X_M * 0.5f; // [DERIVED]
inline constexpr float ELEVATOR_CORE_CENTER_Z_M = WORLD_ORIGIN_Z_M; // [DERIVED]
inline constexpr int ELEVATOR_SHAFT_COUNT = 2; // [ASSUMED] Two illustrative lift shafts.
inline constexpr float ELEVATOR_SHAFT_WIDTH_Z_M = 2.10f; // [ASSUMED] Width of each shaft along the lobby frontage.
inline constexpr float ELEVATOR_SHAFT_CENTER_SPACING_Z_M = 2.30f; // [ASSUMED] Separation between the two shaft centres.
inline constexpr float ELEVATOR_SHAFT_DEPTH_X_M = 3.15f; // [ASSUMED] Shaft depth behind the west-facing lift doors.
inline constexpr float ELEVATOR_DOOR_WIDTH_M = 1.10f; // [ASSUMED] Door opening width.
inline constexpr float ELEVATOR_DOOR_HEIGHT_M = 2.20f; // [ASSUMED] Door height.
inline constexpr float ELEVATOR_DOOR_THICKNESS_M = 0.10f; // [ASSUMED] Door panel thickness.
inline constexpr float ELEVATOR_LOBBY_FLOOR_THICKNESS_M = 0.16f; // [ASSUMED] Lift-lobby slab thickness.
inline constexpr float ELEVATOR_SIGN_WIDTH_M = 0.85f; // [ASSUMED] Visual lift marker size.
inline constexpr float ELEVATOR_SIGN_HEIGHT_M = 0.40f; // [ASSUMED] Visual lift marker size.

// Elevator motion and cabin (illustrative only – not surveyed AEON lift data)
inline constexpr float ELEVATOR_CABIN_WIDTH_Z_M = 1.90f; // [ASSUMED] Cabin width inside shaft.
inline constexpr float ELEVATOR_CABIN_DEPTH_X_M = 2.40f; // [ASSUMED] Cabin depth inside shaft.
inline constexpr float ELEVATOR_CABIN_HEIGHT_M = 2.40f; // [ASSUMED] Cabin interior height.
inline constexpr float ELEVATOR_CABIN_FLOOR_THICKNESS_M = 0.12f; // [ASSUMED] Cabin floor slab.
inline constexpr float ELEVATOR_TRAVEL_SPEED_M_S = 1.20f; // [ASSUMED] Vertical travel speed.
inline constexpr float ELEVATOR_DOOR_OPEN_DURATION_S = 1.40f; // [ASSUMED] Time to fully open doors.
inline constexpr float ELEVATOR_DOOR_CLOSE_DURATION_S = 1.40f; // [ASSUMED] Time to fully close doors.
inline constexpr float ELEVATOR_DWELL_OPEN_DURATION_S = 3.50f; // [ASSUMED] How long doors stay open at a floor.
inline constexpr float ELEVATOR_IDLE_RECALL_DELAY_S = 8.0f; // [ASSUMED] Auto-recall delay when idle.
inline constexpr int ELEVATOR_FLOOR_GROUND = 0; // [DERIVED] Ground / podium level index for lift.
inline constexpr int ELEVATOR_FLOOR_B1 = 1; // [DERIVED]
inline constexpr int ELEVATOR_FLOOR_B2 = 2; // [DERIVED]
inline constexpr float ELEVATOR_LEVEL_Y[3] = {
    GROUND_LEVEL_Y_M,      // 0 – ground
    FLOOR_HEIGHT_B1_M,     // 1 – B1
    FLOOR_HEIGHT_B2_M      // 2 – B2
}; // [DERIVED] Target elevations for the three served levels.

// Second-level circulation is represented by a short connecting ramp from B1.
inline constexpr float B2_UPPER_LEVEL_Y_M = FLOOR_HEIGHT_B1_M; // [DERIVED] B2 ramp starts at B1 level.
inline constexpr float B2_RAMP_DROP_M = FLOOR_HEIGHT_B1_M - FLOOR_HEIGHT_B2_M; // [DERIVED] 3.5 m from the two assumed basement elevations.
inline constexpr float B2_RAMP_LENGTH_X_M = B2_RAMP_DROP_M / RAMP_SLOPE; // [DERIVED] B1-to-B2 ramp length at the assumed 12% grade.
inline constexpr int FLOOR_INDEX_B1 = 1; // [DERIVED] Basement naming convention.
inline constexpr int FLOOR_INDEX_B2 = 2; // [DERIVED] Basement naming convention.

// ------------------------------- Remaining scene dimensions [ASSUMED]
// Named geometry values previously embedded in drawing expressions.
inline constexpr float STREET_ASPHALT_THICKNESS_M = 0.16f; // [ASSUMED] Exterior scene slab.
inline constexpr float SIDEWALK_HEIGHT_M = 0.18f; // [ASSUMED] Exterior scene slab.
inline constexpr float SIDEWALK_EXTRA_LENGTH_X_M = 2.0f; // [ASSUMED] Sidewalk overhang.
inline constexpr float EXTERIOR_GROUND_THICKNESS_M = 0.8f; // [ASSUMED] Continuous terrain slab under exterior objects.
inline constexpr float EXTERIOR_GROUND_SINK_M = 0.01f; // [ASSUMED] Keeps the terrain below road and building surfaces.
inline constexpr float EXTERIOR_GROUND_MARGIN_M = 12.0f; // [ASSUMED] Room for streetscape and neighboring blocks.
inline constexpr float EXTERIOR_RAMP_CLEARANCE_M = 0.05f; // [ASSUMED] Clears terrain around the retaining-wall cap.
inline constexpr float STREET_DASH_START_OFFSET_M = 3.0f; // [ASSUMED] First road dash offset.
inline constexpr float STREET_MARKING_THICKNESS_M = 0.025f; // [ASSUMED] Road marking thickness.
inline constexpr float TREE_TRUNK_BASE_Y_M = 0.0f; // [ASSUMED] Decorative tree baseline.
inline constexpr float TREE_LOWER_CANOPY_CENTER_Y_M = 2.0f; // [ASSUMED] Decorative tree height.
inline constexpr float TREE_UPPER_CANOPY_CENTER_Y_M = 3.1f; // [ASSUMED] Decorative tree height.
inline constexpr float TREE_UPPER_CANOPY_WIDTH_RATIO = 0.68f; // [ASSUMED] Decorative canopy proportion.
inline constexpr float TREE_UPPER_CANOPY_HEIGHT_RATIO = 0.65f; // [ASSUMED] Decorative canopy proportion.
inline constexpr float PODIUM_WINDOW_FRONT_OFFSET_M = 0.16f; // [ASSUMED] Glazing setback.
inline constexpr float PODIUM_WINDOW_SILL_Y_M = 0.48f; // [ASSUMED] Glazing sill height.
inline constexpr float PODIUM_WINDOW_WIDTH_RATIO = 0.86f; // [ASSUMED] Glazing panel width ratio.
inline constexpr float PODIUM_GLASS_DEPTH_M = 0.18f; // [ASSUMED] Glazing thickness.
inline constexpr float PODIUM_RIBBON_BASE_DROP_M = 0.42f; // [ASSUMED] Ribbon vertical offset.
inline constexpr float PODIUM_RIBBON_PHASE_PER_FLOOR = 0.82f; // [ASSUMED] Facade wave phase offset.
inline constexpr float PODIUM_RIBBON_PANEL_OVERHANG_M = 0.12f; // [ASSUMED] Adjacent ribbon overlap.
inline constexpr float ARCH_FRAME_FRONT_OFFSET_M = 0.52f; // [ASSUMED] Arch frame setback.
inline constexpr float ARCH_GLASS_FRONT_OFFSET_M = 0.24f; // [ASSUMED] Entry glazing setback.
inline constexpr float ARCH_GLASS_DEPTH_M = 0.20f; // [ASSUMED] Entry glazing thickness.
inline constexpr float ARCH_SEGMENT_LENGTH_RATIO = 1.12f; // [ASSUMED] Faceted arch overlap.
inline constexpr float SIGN_FRONT_OFFSET_M = 0.40f; // [ASSUMED] Sign backing setback.
inline constexpr float SIGN_BACKING_WIDTH_M = 0.73f; // [ASSUMED] Sign backing panel width.
inline constexpr float SIGN_BACKING_HEIGHT_M = 0.035f; // [ASSUMED] Sign backing panel height.
inline constexpr float SIGN_BACKING_THICKNESS_M = 0.37f; // [ASSUMED] Sign backing panel depth.
inline constexpr float SIGN_BOTTOM_OFFSET_M = 0.60f; // [ASSUMED] Sign text baseline offset.
inline constexpr float SIGN_TEXT_FRONT_OFFSET_M = 0.64f; // [ASSUMED] Sign text stand-off.
inline constexpr float TOWER_GLASS_FRONT_OFFSET_M = 0.14f; // [ASSUMED] Tower glazing setback.
inline constexpr float TOWER_GLASS_DEPTH_M = 0.18f; // [ASSUMED] Tower glazing thickness.
inline constexpr float TOWER_WINDOW_SIDE_MARGIN_M = 1.0f; // [ASSUMED] Tower window margins.
inline constexpr float TOWER_MULLION_FRONT_OFFSET_M = 0.25f; // [ASSUMED] Mullion position.
inline constexpr float TOWER_MULLION_DEPTH_M = 0.12f; // [ASSUMED] Mullion thickness.
inline constexpr float TOWER_BALCONY_PROJECTION_RATIO = 0.35f; // [ASSUMED] Balcony projection ratio.
inline constexpr float TOWER_BALCONY_EXTRA_WIDTH_M = 0.45f; // [ASSUMED] Balcony width margin.
inline constexpr float TOWER_BALCONY_EXTRA_DEPTH_M = 0.45f; // [ASSUMED] Balcony depth margin.
inline constexpr float TOWER_CROWN_FRONT_OFFSET_M = 0.10f; // [ASSUMED] Crown setback.
inline constexpr float TOWER_CROWN_EXTRA_WIDTH_M = 0.60f; // [ASSUMED] Crown width margin.
inline constexpr float TOWER_CROWN_HEIGHT_M = 0.45f; // [ASSUMED] Crown height.
inline constexpr float TOWER_CROWN_DEPTH_M = 0.80f; // [ASSUMED] Crown depth.
inline constexpr float BILLBOARD_FRONT_OFFSET_M = 0.40f; // [ASSUMED] Sign-face offset.
inline constexpr float BILLBOARD_HEIGHT_M = 0.37f; // [ASSUMED] Sign panel height.
inline constexpr float BILLBOARD_DEPTH_M = 0.035f; // [ASSUMED] Sign panel thickness.
inline constexpr float RAMP_RETAINING_CAP_THICKNESS_M = 0.12f; // [ASSUMED] Ramp wall cap thickness.
inline constexpr float RAMP_RETAINING_CAP_OVERHANG_M = 0.12f; // [ASSUMED] Ramp wall cap overhang.
inline constexpr float RAMP_PORTAL_SIDE_PIER_CENTER_RATIO = 0.5f; // [DERIVED] Symmetric portal pier placement.
inline constexpr float RAMP_FASCIA_VERTICAL_OFFSET_M = 0.05f; // [ASSUMED] Entrance fascia offset.
inline constexpr float RAMP_BARRIER_POST_HEIGHT_M = 1.0f; // [ASSUMED] Gate post height.
inline constexpr float HUD_EXTERIOR_WIDTH_PX = 370.0f; // [ASSUMED] Exterior HUD width.
inline constexpr float HUD_EXTERIOR_HEIGHT_PX = 142.0f; // [ASSUMED] Exterior HUD height.
inline constexpr float HUD_PANEL_LINE_BOTTOM_PADDING_PX = 43.0f; // [ASSUMED] Text anchor.
inline constexpr float HUD_PANEL_HEIGHT_WITH_EXTENSIONS_PX = 450.0f; // [ASSUMED] Expanded basement HUD height.
inline constexpr float FRAME_LIGHT_DIR_X = -0.45f; // [ASSUMED] Rendering light direction.
inline constexpr float FRAME_LIGHT_DIR_Y = 1.0f; // [ASSUMED] Rendering light direction.
inline constexpr float FRAME_LIGHT_DIR_Z = 0.35f; // [ASSUMED] Rendering light direction.

// -------------------------------- Simulation-only parameters [ASSUMED]
inline constexpr int RAMP_QUEUE_EXTRA_POSITIONS = 1; // [ASSUMED] One safe waiting position beyond the calculated ramp length.
inline constexpr float AUTO_VEHICLE_SPEED_MPS = 3.6f; // [ASSUMED] Animation speed, not measured AEON traffic speed.
inline constexpr float BAY_MANEUVER_SPEED_MPS = 2.1f; // [ASSUMED] Animation speed.
inline constexpr float EXIT_MANEUVER_SPEED_MPS = 1.8f; // [ASSUMED] Animation speed.
inline constexpr float MANUAL_DRIVE_SPEED_MPS = 5.0f; // [ASSUMED] User-control speed.
inline constexpr float MANUAL_TURN_SPEED_RAD_S = 1.55f; // [ASSUMED] User-control steering rate.
inline constexpr float PARKING_DWELL_MIN_SECONDS = 90.0f; // [ASSUMED] Random demo dwell lower bound, not a real customer stay.
inline constexpr float PARKING_DWELL_MAX_SECONDS = 120.0f; // [ASSUMED] Random demo dwell upper bound, not a real customer stay.
inline constexpr float SPAWN_INTERVAL_SECONDS = 1.5f; // [ASSUMED] Demo arrival interval; supports FULL-state demonstration.
inline constexpr int MAX_ACTIVE_AUTO_VEHICLES = TOTAL_ZONE_A_CAR_SLOTS; // [DERIVED] Cannot exceed the 64 simulated car spaces.
inline constexpr float ENTRY_GATE_PREOPEN_SECONDS = 0.35f; // [ASSUMED] Animation delay.
inline constexpr float EXIT_PAYMENT_WAIT_SECONDS = 2.0f; // [ASSUMED] Demo wait to represent assisted mobile payment; no payment transaction is processed.
inline constexpr float BARRIER_OPEN_DURATION_SECONDS = 4.0f; // [ASSUMED] Animation duration.
inline constexpr float ROUTE_ARRIVAL_EPSILON_M = 0.18f; // [ASSUMED] Waypoint arrival tolerance.
inline constexpr float RAMP_COLLISION_VERTICAL_TOLERANCE_M = 1.6f; // [ASSUMED] Ignore vehicles at clearly different ramp elevations.
inline constexpr float PLAYER_BOUNDARY_MARGIN_M = CAR_LENGTH_M * 0.5f; // [DERIVED] Keep player model within the perimeter.
inline constexpr float CAMERA_MOUSE_SENSITIVITY = 0.006f; // [ASSUMED] Input sensitivity.
inline constexpr float CAMERA_MIN_PITCH_RAD = 0.12f; // [ASSUMED] Orbit camera limit.
inline constexpr float CAMERA_MAX_PITCH_RAD = 1.48f; // [ASSUMED] Orbit camera limit.
inline constexpr float CAMERA_ZOOM_IN_FACTOR = 0.92f; // [ASSUMED] Mouse-wheel scale.
inline constexpr float CAMERA_ZOOM_OUT_FACTOR = 1.08f; // [ASSUMED] Mouse-wheel scale.
inline constexpr float CAMERA_TARGET_HEIGHT_FACTOR = 0.45f; // [ASSUMED] Camera target placement fraction.
inline constexpr float BASEMENT_CAMERA_TARGET_Y_M =
    FLOOR_HEIGHT_B1_M + FLOOR_TO_CEILING_M * CAMERA_TARGET_HEIGHT_FACTOR; // [DERIVED]
inline constexpr float CAMERA_TOP_VIEW_Z_OFFSET_M = 0.01f; // [ASSUMED] Avoid singular look direction.
inline constexpr float CAMERA_FOV_DEG = 48.0f; // [ASSUMED] Perspective camera field of view.
inline constexpr float CAMERA_NEAR_CLIP_M = 0.1f; // [ASSUMED] Projection clipping plane.
inline constexpr float CAMERA_FAR_CLIP_M = 220.0f; // [ASSUMED] Projection clipping plane.
inline constexpr float HUD_BAR_HEIGHT_PX = 18.0f; // [ASSUMED] HUD visual layout.
inline constexpr float HUD_TEXT_OFFSET_X_PX = 14.0f; // [ASSUMED] HUD visual layout.
inline constexpr float HUD_TEXT_START_Y_PX = 43.0f; // [ASSUMED] HUD visual layout.
inline constexpr float HUD_LINE_STEP_PX = 22.0f; // [ASSUMED] HUD visual layout.
inline constexpr float ARROW_HEAD_ANGLE_RAD = 0.68f; // [ASSUMED] Stylised road-arrow geometry.
inline constexpr float ARROW_SHAFT_OFFSET_RATIO = 0.20f; // [ASSUMED] Stylised road-arrow geometry.
inline constexpr float ARROW_SHAFT_LENGTH_RATIO = 0.58f; // [ASSUMED] Stylised road-arrow geometry.
inline constexpr float ARROW_HEAD_OFFSET_RATIO = 0.18f; // [ASSUMED] Stylised road-arrow geometry.
inline constexpr float ARROW_HEAD_SIDE_RATIO = 0.10f; // [ASSUMED] Stylised road-arrow geometry.
inline constexpr float ARROW_HEAD_LENGTH_RATIO = 0.34f; // [ASSUMED] Stylised road-arrow geometry.
inline constexpr float BAY_FILL_FLOOR_OFFSET_M = 0.004f; // [ASSUMED] Prevent visual z-fighting.
inline constexpr float BAY_FILL_HEIGHT_FACTOR = 0.45f; // [ASSUMED] Visual fill height relative to marking thickness.
inline constexpr float WINDOW_REFRESH_DT_CAP_SECONDS = 0.04f; // [ASSUMED] Simulation frame-step cap.
inline constexpr int OPENGL_CONTEXT_MAJOR = 3; // [ASSUMED] Requested graphics context version.
inline constexpr int OPENGL_CONTEXT_MINOR = 3; // [ASSUMED] Requested graphics context version.
inline constexpr int MULTISAMPLE_SAMPLES = 4; // [ASSUMED] Visual anti-aliasing setting.
inline constexpr float CAMERA_ROTATION_DELTA_SCALE = CAMERA_MOUSE_SENSITIVITY; // [DERIVED] Same value for mouse X/Y rotation.

// -------------------------------- UI and camera [ASSUMED]
inline constexpr int WINDOW_WIDTH_PX = 1440; // [ASSUMED] Application window setting.
inline constexpr int WINDOW_HEIGHT_PX = 900; // [ASSUMED] Application window setting.
inline constexpr float CAMERA_INITIAL_YAW_RAD = -PI * 0.5f; // [ASSUMED] Face the mall's public frontage on the negative-Z side.
inline constexpr float CAMERA_INITIAL_PITCH_RAD = 0.42f; // [ASSUMED] Low three-quarter angle that keeps the facade visible.
inline constexpr float CAMERA_INITIAL_DISTANCE_M = 78.0f; // [ASSUMED] Frame the mall and compact exterior context, without the full-size ramp.
inline constexpr float CAMERA_BASEMENT_INITIAL_YAW_RAD = -0.95f; // [ASSUMED] B1 three-quarter inspection angle.
inline constexpr float CAMERA_BASEMENT_INITIAL_PITCH_RAD = 0.92f; // [ASSUMED] Look down into the open-top B1 cutaway.
inline constexpr float CAMERA_BASEMENT_INITIAL_DISTANCE_M = 78.0f; // [ASSUMED] Frame Zone A plus the assumed bike/lift annexes.
inline constexpr float CAMERA_BASEMENT_TARGET_X_M =
    (ZONE_MIN_X_M + ELEVATOR_CORE_CENTER_X_M) * GEOMETRY_HALF_RATIO; // [DERIVED]
inline constexpr float CAMERA_BASEMENT_TARGET_Z_M =
    BIKE_ZONE_CENTER_Z_M * GEOMETRY_HALF_RATIO; // [DERIVED] Include north-side bike demonstration bays.
inline constexpr float CAMERA_BASEMENT_TOP_VIEW_HEIGHT_M = 58.0f; // [ASSUMED] Top-down inspection height for B1.
inline constexpr float CAMERA_MIN_DISTANCE_M = 20.0f; // [ASSUMED] Camera zoom limit.
inline constexpr float CAMERA_MAX_DISTANCE_M = 110.0f; // [ASSUMED] Camera zoom limit.
inline constexpr float CAMERA_TOP_VIEW_HEIGHT_M = 85.0f; // [ASSUMED] Top-down camera height.
inline constexpr float HUD_WIDTH_PX = 510.0f; // [ASSUMED] HUD layout dimension.
inline constexpr float HUD_HEIGHT_PX = 322.0f; // [ASSUMED] HUD layout dimension.
inline constexpr float HUD_MARGIN_PX = 16.0f; // [ASSUMED] HUD screen margin.
inline constexpr float HUD_TEXT_PIXEL_SIZE = 1.65f; // [ASSUMED] Built-in bitmap font scale.
inline constexpr float BITMAP_GLYPH_PIXEL_FILL_RATIO = 0.78f; // [ASSUMED] Glyph-cell fill ratio for world-space mall lettering.
inline constexpr float BITMAP_GLYPH_DEPTH_M = 0.07f; // [ASSUMED] World-space lettering extrusion depth.
inline constexpr int BITMAP_GLYPH_WIDTH_CELLS = 5; // [DERIVED] Five-column bitmap glyph.
inline constexpr int BITMAP_GLYPH_HEIGHT_CELLS = 7; // [DERIVED] Seven-row bitmap glyph.
inline constexpr int BITMAP_GLYPH_LAST_ROW_INDEX = BITMAP_GLYPH_HEIGHT_CELLS - 1; // [DERIVED] Zero-based final row index.
inline constexpr int BITMAP_GLYPH_ADVANCE_CELLS = 6; // [ASSUMED] One blank column between glyphs.
inline constexpr float RAMP_STRIPE_FACE_OFFSET_Y_M = 0.015f; // [ASSUMED] Small visual lift to avoid z-fighting.
inline constexpr float RAMP_STRIPE_THICKNESS_SCALE = 1.08f; // [ASSUMED] Slight stripe oversize to cover the barrier arm.
inline constexpr float RAMP_STRIPE_LENGTH_SCALE = 0.55f; // [ASSUMED] Stripe segment depth ratio.
inline constexpr float HUD_ORTHO_NEAR_CLIP = -10.0f; // [ASSUMED] 2D overlay projection depth.
inline constexpr float HUD_ORTHO_FAR_CLIP = 10.0f; // [ASSUMED] 2D overlay projection depth.
inline constexpr float HUD_RECT_DEPTH_M = 0.02f; // [ASSUMED] Flat overlay panel depth.
inline constexpr float HUD_GLYPH_PIXEL_FILL_RATIO = 0.88f; // [ASSUMED] Pixel-glyph fill ratio.
inline constexpr float BIKE_SEAT_Z_OFFSET_RATIO = 0.12f; // [ASSUMED] Seat position within simplified bike body.
inline constexpr float BIKE_WHEEL_THICKNESS_RATIO = 0.55f; // [ASSUMED] Box-wheel depth ratio for the simplified bike model.

// ------------------------------- Coordinate helper functions [DERIVED]
constexpr float slotCenterX(int slotIndex) {
    return CORE_MIN_X_M + (static_cast<float>(slotIndex) + 0.5f) * STALL_W_M;
}
constexpr float vehicleBayCenterX(int slotIndex) {
    const int withinColumnSpan = slotIndex % STALLS_PER_COLUMN_SPAN;
    const float offset = (slotIndex > 0 && withinColumnSpan == 0)
        ? SLOT_COLUMN_CLEARANCE_OFFSET_M
        : (withinColumnSpan == STALLS_PER_COLUMN_SPAN - 1
            ? -SLOT_COLUMN_CLEARANCE_OFFSET_M : 0.0f);
    return slotCenterX(slotIndex) + offset;
}
constexpr float slotCenterZ(float aisleCenterZ, int rowSide) {
    return aisleCenterZ + static_cast<float>(rowSide) * (AISLE_W_M * 0.5f + STALL_L_M * 0.5f);
}
constexpr float inboundTrackZ(float aisleCenterZ) {
    return aisleCenterZ - INBOUND_LANE_OFFSET_M;
}
constexpr float outboundTrackZ(float aisleCenterZ) {
    return aisleCenterZ + INBOUND_LANE_OFFSET_M;
}
inline float rampYAtX(float x, float upperLevelY, float lowerLevelY) {
    const float t = (x - RAMP_START_X_M) / RAMP_LENGTH_X_M;
    const float clampedT = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
    return upperLevelY + clampedT * (lowerLevelY - upperLevelY);
}
inline float rampYAtX(float x) {
    return rampYAtX(x, GROUND_LEVEL_Y_M, FLOOR_HEIGHT_B1_M);
}

constexpr float motorbikeSlotX(int slotIndex) {
    return BIKE_ZONE_CENTER_X_M +
        (static_cast<float>(slotIndex) -
         (static_cast<float>(BIKE_STALLS_PER_ROW) - 1.0f) * 0.5f) * BIKE_STALL_W_M;
}
constexpr float motorbikeSlotZ(int rowIndex) {
    const float rowPitch = BIKE_STALL_L_M + BIKE_ROW_AISLE_W_M;
    return BIKE_ZONE_CENTER_Z_M +
        (static_cast<float>(rowIndex) -
         (static_cast<float>(BIKE_ROW_COUNT) - 1.0f) * 0.5f) * rowPitch;
}

static_assert(TOTAL_ZONE_A_CAR_SLOTS == 64, "Zone A must contain 64 car spaces.");
static_assert(COLUMN_SPACING_X_M == 7.5f, "Column spacing must remain 7.5 m.");
static_assert(BASEMENT_FLOORS == 2, "The complex has two basement levels.");
static_assert(TOTAL_ZONE_A_BIKE_SLOTS == 24, "Motorbike demo area must contain 24 sample spaces.");
static_assert(ELEVATOR_SHAFT_COUNT == 2, "The illustrative elevator core has two shafts.");
static_assert(B2_RAMP_LENGTH_X_M == RAMP_LENGTH_X_M,
              "The current shared ramp geometry assumes equal ground-to-B1 and B1-to-B2 drops.");

} // namespace AeonLayout
