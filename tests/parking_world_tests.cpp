#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "../parking_world.h"

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void testLayoutAndMotorbikeCapacity() {
    ParkingWorld b1(AeonLayout::FLOOR_HEIGHT_B1_M, AeonLayout::FLOOR_INDEX_B1, 101u);
    b1.initialise();
    require(static_cast<int>(b1.slots().size()) == AeonLayout::TOTAL_ZONE_A_CAR_SLOTS,
            "B1 must generate all 64 car stalls");
    require(static_cast<int>(b1.columns().size()) == 20,
            "B1 must generate the expected 20 columns");
    require(static_cast<int>(b1.motorbikeSlots().size()) == AeonLayout::TOTAL_ZONE_A_BIKE_SLOTS,
            "Motorbike annex must generate all demo slots");
    require(b1.motorbikeCount() == AeonLayout::INITIAL_PARKED_MOTORBIKES,
            "Initial motorbike demo population mismatch");
    require(b1.freeMotorbikeSlotCount() == AeonLayout::TOTAL_ZONE_A_BIKE_SLOTS -
                                               AeonLayout::INITIAL_PARKED_MOTORBIKES,
            "Motorbike free-space count mismatch");

    while (b1.spawnMotorbike()) {}
    require(b1.motorbikeCount() == AeonLayout::TOTAL_ZONE_A_BIKE_SLOTS,
            "Motorbike annex did not reach its configured capacity");
    require(b1.freeMotorbikeSlotCount() == 0,
            "Motorbike annex should have no free slots at capacity");
    require(!b1.spawnMotorbike(), "Motorbike spawn should fail when the annex is full");
    require(b1.removeLastMotorbike(), "Removing the last motorbike should succeed");
    require(b1.freeMotorbikeSlotCount() == 1,
            "Removing a motorbike should release exactly one space");
}

void testB2GeometryAndIndependentWorld() {
    ParkingWorld b1(AeonLayout::FLOOR_HEIGHT_B1_M, AeonLayout::FLOOR_INDEX_B1, 101u);
    ParkingWorld b2(AeonLayout::FLOOR_HEIGHT_B2_M, AeonLayout::FLOOR_INDEX_B2, 202u);
    b1.initialise();
    b2.initialise();
    require(b2.floorIndex() == AeonLayout::FLOOR_INDEX_B2, "B2 floor index mismatch");
    require(static_cast<int>(b2.slots().size()) == AeonLayout::TOTAL_ZONE_A_CAR_SLOTS,
            "B2 must generate its own independent 64-car demo inventory");
    require(static_cast<int>(b2.motorbikeSlots().size()) == AeonLayout::TOTAL_ZONE_A_BIKE_SLOTS,
            "B2 must generate its own motorbike demo annex");
    require(std::abs(b2.floorY() - AeonLayout::FLOOR_HEIGHT_B2_M) < 0.001f,
            "B2 floor elevation mismatch");
    require(std::abs(b2.upperFloorY() - AeonLayout::FLOOR_HEIGHT_B1_M) < 0.001f,
            "B2 ramp must connect from B1 level");
    require(std::abs(b2.slots().front().center().y - AeonLayout::FLOOR_HEIGHT_B2_M) < 0.001f,
            "B2 parking stalls must be placed at B2 level");
    const float midpoint = AeonLayout::rampYAtX(AeonLayout::RAMP_MID_X_M,
                                                 b2.upperFloorY(), b2.floorY());
    const float expectedMidpoint = (AeonLayout::FLOOR_HEIGHT_B1_M +
                                    AeonLayout::FLOOR_HEIGHT_B2_M) *
                                   AeonLayout::GEOMETRY_HALF_RATIO;
    require(std::abs(midpoint - expectedMidpoint) < 0.01f,
            "B1-to-B2 ramp midpoint elevation mismatch");
    require(b2.spawnVehicle(), "B2 should accept a car when a slot is free");
    require(b2.reservedSlotCount() == 1 && b2.occupiedSlotCount() == 0,
            "A new B2 car must reserve a slot before it physically parks");
    require(std::abs(b2.vehicles().front().position().y - b2.upperFloorY()) < 0.001f,
            "B2 car should spawn at the upper B1 connection");
    bool b2VehicleReachedFloor = false;
    for (int frame = 0; frame < 3000; ++frame) {
        b2.update(0.04f);
        require(!b2.hasTrafficCollisions(), "B2 route produced a vehicle collision");
        require(!b2.hasColumnCollisions(), "B2 route produced a car-to-column collision");
        for (const auto& vehicle : b2.vehicles()) {
            if (std::abs(vehicle.position().y - AeonLayout::FLOOR_HEIGHT_B2_M) < 0.05f)
                b2VehicleReachedFloor = true;
        }
        if (b2VehicleReachedFloor && b2.occupiedSlotCount() > 0) break;
    }
    require(b2VehicleReachedFloor && b2.occupiedSlotCount() > 0,
            "B2 car did not descend from B1 and complete its parking maneuver");
    require(b1.freeSlotCount() == AeonLayout::TOTAL_ZONE_A_CAR_SLOTS,
            "B1 and B2 worlds must maintain independent parking inventories");
}

void testAnnexAndElevatorGeometryBounds() {
    // The two annexes are illustrative [ASSUMED] geometry. These checks verify
    // that their generated openings/shafts fit their own configured envelopes.
    require(AeonLayout::BIKE_ZONE_ACCESS_OPENING_WIDTH_X_M >=
                AeonLayout::BIKE_ZONE_WIDTH_X_M,
            "Motorbike access opening must cover its annex width");
    require(AeonLayout::ELEVATOR_OPENING_WIDTH_Z_M >=
                AeonLayout::ELEVATOR_WALKWAY_WIDTH_M,
            "Elevator access opening must cover its walkway width");
    require(AeonLayout::ELEVATOR_SHAFT_COUNT == 2,
            "The sample elevator core must contain two shafts");
    require(AeonLayout::ELEVATOR_CORE_CENTER_X_M -
                AeonLayout::ELEVATOR_CORE_WIDTH_X_M * AeonLayout::GEOMETRY_HALF_RATIO >=
                AeonLayout::ZONE_MAX_X_M,
            "Elevator core should not overlap the car-zone perimeter");
    require(AeonLayout::BIKE_ZONE_CENTER_Z_M -
                AeonLayout::BIKE_ZONE_DEPTH_Z_M * AeonLayout::GEOMETRY_HALF_RATIO >=
                AeonLayout::ZONE_MAX_Z_M,
            "Motorbike annex should be outside the car-zone north edge");

    const float bikeHalfX = AeonLayout::BIKE_STALL_W_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float bikeHalfZ = AeonLayout::BIKE_STALL_L_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float bikeMinX = AeonLayout::BIKE_ZONE_CENTER_X_M -
                           AeonLayout::BIKE_ZONE_WIDTH_X_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float bikeMaxX = AeonLayout::BIKE_ZONE_CENTER_X_M +
                           AeonLayout::BIKE_ZONE_WIDTH_X_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float bikeMinZ = AeonLayout::BIKE_ZONE_CENTER_Z_M -
                           AeonLayout::BIKE_ZONE_DEPTH_Z_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float bikeMaxZ = AeonLayout::BIKE_ZONE_CENTER_Z_M +
                           AeonLayout::BIKE_ZONE_DEPTH_Z_M * AeonLayout::GEOMETRY_HALF_RATIO;
    for (int row = 0; row < AeonLayout::BIKE_ROW_COUNT; ++row) {
        for (int index = 0; index < AeonLayout::BIKE_STALLS_PER_ROW; ++index) {
            const float x = AeonLayout::motorbikeSlotX(index);
            const float z = AeonLayout::motorbikeSlotZ(row);
            require(x - bikeHalfX >= bikeMinX - 0.001f && x + bikeHalfX <= bikeMaxX + 0.001f,
                    "Motorbike stall exceeds the annex X envelope");
            require(z - bikeHalfZ >= bikeMinZ - 0.001f && z + bikeHalfZ <= bikeMaxZ + 0.001f,
                    "Motorbike stall exceeds the annex Z envelope");
        }
    }

    const float coreHalfZ = AeonLayout::ELEVATOR_CORE_DEPTH_Z_M *
                            AeonLayout::GEOMETRY_HALF_RATIO;
    const float shaftHalfZ = AeonLayout::ELEVATOR_SHAFT_WIDTH_Z_M *
                             AeonLayout::GEOMETRY_HALF_RATIO;
    for (int shaft = 0; shaft < AeonLayout::ELEVATOR_SHAFT_COUNT; ++shaft) {
        const float shaftCenterZ = AeonLayout::ELEVATOR_CORE_CENTER_Z_M +
            (static_cast<float>(shaft) -
             static_cast<float>(AeonLayout::ELEVATOR_SHAFT_COUNT - 1) *
                 AeonLayout::GEOMETRY_HALF_RATIO) *
                AeonLayout::ELEVATOR_SHAFT_CENTER_SPACING_Z_M;
        require(shaftCenterZ - shaftHalfZ >=
                    AeonLayout::ELEVATOR_CORE_CENTER_Z_M - coreHalfZ - 0.001f,
                "Elevator shaft extends beyond the core's south edge");
        require(shaftCenterZ + shaftHalfZ <=
                    AeonLayout::ELEVATOR_CORE_CENTER_Z_M + coreHalfZ + 0.001f,
                "Elevator shaft extends beyond the core's north edge");
    }

    const float columnHalf = AeonLayout::COLUMN_SIZE_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float carHalfWidth = AeonLayout::CAR_WIDTH_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float nearestBayCenterDistance = AeonLayout::STALL_W_M *
                                           AeonLayout::GEOMETRY_HALF_RATIO +
                                           AeonLayout::SLOT_COLUMN_CLEARANCE_OFFSET_M;
    require(nearestBayCenterDistance > columnHalf + carHalfWidth,
            "Assumed stall offset must leave the model car envelope clear of a column");
}

void testSafeMultiVehicleTrafficAndBarriers() {
    ParkingWorld world(AeonLayout::FLOOR_HEIGHT_B1_M, AeonLayout::FLOOR_INDEX_B1, 303u);
    world.initialise();
    for (int i = 0; i < 5; ++i) {
        require(world.spawnVehicle(), "Could not spawn initial traffic vehicle");
    }
    require(world.carCount() == 5, "All five test vehicles should be present at the start");
    require(world.reservedSlotCount() == 5 && world.occupiedSlotCount() == 0,
            "All five incoming cars should reserve distinct free stalls");
    require(!world.hasTrafficCollisions(), "Initial vehicle queue overlaps");

    bool entryBarrierSeen = false;
    bool paymentQueueSeen = false;
    bool exitBarrierSeen = false;
    bool occupiedSeen = false;
    bool entryConnectorSeen = false;
    bool exitConnectorSeen = false;
    for (int frame = 0; frame < 9000; ++frame) {
        world.update(0.04f);
        for (const auto& vehicle : world.vehicles()) {
            if (vehicle.phase() == Vehicle::Phase::ToEntryConnector ||
                vehicle.phase() == Vehicle::Phase::ToTargetAisle)
                entryConnectorSeen = true;
            if (vehicle.phase() == Vehicle::Phase::ToExitConnector ||
                vehicle.phase() == Vehicle::Phase::ToExitLane)
                exitConnectorSeen = true;
        }
        if (world.entryBarrierOpenTimer() > 0.0f) entryBarrierSeen = true;
        if (world.paymentQueueCount() > 0) paymentQueueSeen = true;
        if (world.exitBarrierOpenTimer() > 0.0f) exitBarrierSeen = true;
        if (world.occupiedSlotCount() > 0) occupiedSeen = true;
        require(!world.hasTrafficCollisions(),
                "Vehicle-to-vehicle collision detected during entry/parking/exit cycle");
        require(!world.hasColumnCollisions(),
                "Vehicle-to-column collision detected during entry/parking/exit cycle");
        if (entryBarrierSeen && paymentQueueSeen && exitBarrierSeen && occupiedSeen) break;
    }
    require(entryBarrierSeen, "Entry barrier never opened during the test");
    require(paymentQueueSeen, "No vehicle reached the payment queue");
    require(exitBarrierSeen, "Exit barrier never opened after simulated payment");
    require(occupiedSeen, "No parking stall reached OCCUPIED state");
    require(entryConnectorSeen && exitConnectorSeen,
            "Simulation did not exercise the entry and exit connector paths");
}

void testIntersectionCoverageAndFullState(std::uint32_t seed) {
    ParkingWorld world(AeonLayout::FLOOR_HEIGHT_B1_M, AeonLayout::FLOOR_INDEX_B1, seed);
    world.initialise();
    bool sawSecondAisle = false;
    bool sawPositiveSideRow = false;
    bool sawNegativeSideRow = false;
    bool sawStagingManeuver = false;
    bool sawReverseIntoBay = false;
    bool sawOccupiedState = false;
    bool fullSeen = false;

    // Keep automatic arrivals enabled; this drives both aisles, both row sides,
    // turning/staging, reverse parking, barrier flow, and the 64-space FULL state.
    for (int frame = 0; frame < 5000; ++frame) {
        world.update(0.04f);
        world.updateSpawning(0.04f);

        for (const auto& vehicle : world.vehicles()) {
            if (vehicle.slotIndex() < 0 ||
                vehicle.slotIndex() >= static_cast<int>(world.slots().size())) continue;
            const ParkingSlot& target = world.slots()[static_cast<std::size_t>(vehicle.slotIndex())];
            if (std::abs(target.aisleZ() - AeonLayout::AISLE_CENTER_Z_M[1]) < 0.001f)
                sawSecondAisle = true;
            if (target.rowSide() > 0) sawPositiveSideRow = true;
            if (target.rowSide() < 0) sawNegativeSideRow = true;
            if (vehicle.phase() == Vehicle::Phase::ToBayStaging)
                sawStagingManeuver = true;
            if (vehicle.phase() == Vehicle::Phase::ReversingIntoBay)
                sawReverseIntoBay = true;

        }
        if (world.occupiedSlotCount() > 0) sawOccupiedState = true;
        require(!world.hasTrafficCollisions(),
                "Vehicle collision detected during sustained multi-car/intersection simulation");
        require(!world.hasColumnCollisions(),
                "Vehicle-to-column collision detected during sustained multi-car simulation");

        if (world.freeSlotCount() == 0) {
            fullSeen = true;
            require(world.reservedSlotCount() + world.occupiedSlotCount() ==
                        AeonLayout::TOTAL_ZONE_A_CAR_SLOTS,
                    "FULL state must account for every car stall as reserved or occupied");
            require(!world.spawnVehicle(),
                    "A new car must not be admitted when all stalls are reserved/occupied");
            break;
        }
    }

    require(sawSecondAisle, "Simulation did not exercise the second aisle");
    require(sawPositiveSideRow && sawNegativeSideRow,
            "Simulation did not exercise both sides of the parking rows");
    require(sawStagingManeuver, "No vehicle reached the staging point before turning into a bay");
    require(sawReverseIntoBay, "No vehicle exercised reverse parking");
    require(sawOccupiedState, "No vehicle reached OCCUPIED state during sustained traffic");
    require(fullSeen, "The 64-space Zone A simulation did not reach FULL under sustained arrivals");
}
}

int main() {
    try {
        testLayoutAndMotorbikeCapacity();
        testB2GeometryAndIndependentWorld();
        testAnnexAndElevatorGeometryBounds();
        testSafeMultiVehicleTrafficAndBarriers();
        // Repeat the sustained-arrival / two-aisle / barrier / FULL scenario
        // under fixed seeds to make the regression test reproducible.
        testIntersectionCoverageAndFullState(20261118u);
        testIntersectionCoverageAndFullState(20261119u);
        testIntersectionCoverageAndFullState(20261120u);
        std::cout << "All AEON parking world tests passed.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "AEON parking world test failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
