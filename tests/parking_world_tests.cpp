#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <string>

#define AEON_PARKING_WORLD_TEST_ACCESS
#include "../parking_world.h"


struct ParkingWorldTestAccess {
    static void fillAllStallsAsParked(ParkingWorld& world) {
        world.traffic_.clear();
        for (std::size_t index = 0; index < world.slots_.size(); ++index) {
            ParkingSlot& slot = world.slots_[index];
            Vehicle vehicle(world.nextVehicleId_++, slot.center(), static_cast<int>(index));
            vehicle.yaw_ = slot.facingAisleYaw();
            vehicle.phase_ = Vehicle::Phase::Parked;
            vehicle.phaseTimer_ = 0.0f;
            vehicle.parkedSeconds_ = 0.0f;
            slot.occupy(vehicle.id_);
            world.traffic_.push_back(vehicle);
        }
    }
};

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

const char* vehiclePhaseName(Vehicle::Phase phase) {
    static const char* names[] = {
        "EnteringRamp", "EntryGateWait", "ToEntryConnector", "ToTargetAisle",
        "ToBayApproach", "ToBayStaging", "ReversingIntoBay", "Parked",
        "LeavingBay", "AligningOutboundLane", "ToExitConnector", "ToExitLane",
        "ExitGateApproach", "ExitPaymentWait", "ExitingRamp"
    };
    const auto index = static_cast<std::size_t>(phase);
    return index < sizeof(names) / sizeof(names[0]) ? names[index] : "Unknown";
}

bool isPermittedTrafficWait(Vehicle::Phase phase) {
    return phase == Vehicle::Phase::Parked ||
           phase == Vehicle::Phase::EntryGateWait ||
           phase == Vehicle::Phase::ExitPaymentWait;
}

struct VehicleWatchState {
    Vehicle::Phase phase = Vehicle::Phase::EnteringRamp;
    float lastMovementSeconds = 0.0f;
};

std::string vehicleDiagnostic(const std::string& context, int seed, int count,
                              const Vehicle& vehicle, const char* reason,
                              float simSeconds) {
    std::ostringstream message;
    message << context << " seed=" << seed << " N=" << count
            << " t=" << simSeconds << "s id=" << vehicle.id()
            << " phase=" << vehiclePhaseName(vehicle.phase())
            << " pos=(" << vehicle.position().x << ',' << vehicle.position().y
            << ',' << vehicle.position().z << ") reason=" << reason;
    return message.str();
}

void verifyTrafficFrame(
    const ParkingWorld& world, float simSeconds, float dt,
    const std::unordered_map<int, glm::vec3>& previousPositions,
    std::unordered_map<int, VehicleWatchState>& watch,
    const std::string& context, int seed, int count) {
    if (world.hasTrafficCollisions()) {
        require(false, context + " seed=" + std::to_string(seed) + " N=" +
                           std::to_string(count) + " produced a vehicle-to-vehicle collision");
    }
    if (world.hasColumnCollisions()) {
        require(false, context + " seed=" + std::to_string(seed) + " N=" +
                           std::to_string(count) + " produced a vehicle-to-column collision");
    }

    for (const auto& vehicle : world.vehicles()) {
        auto state = watch.find(vehicle.id());
        if (state == watch.end()) {
            state = watch.emplace(vehicle.id(), VehicleWatchState{vehicle.phase(), simSeconds}).first;
        } else if (state->second.phase != vehicle.phase()) {
            state->second.phase = vehicle.phase();
            state->second.lastMovementSeconds = simSeconds;
        }

        const auto previous = previousPositions.find(vehicle.id());
        if (previous != previousPositions.end()) {
            const glm::vec3 delta = vehicle.position() - previous->second;
            const float displacement = std::sqrt(delta.x * delta.x + delta.y * delta.y +
                                                 delta.z * delta.z);
            const float allowed = AeonLayout::AUTO_VEHICLE_SPEED_MPS * dt * 1.05f + 1.0e-4f;
            if (displacement > allowed) {
                require(false, vehicleDiagnostic(context, seed, count, vehicle,
                                                  "position jump exceeds speed*dt*1.05", simSeconds));
            }
            if (displacement > 1.0e-5f) state->second.lastMovementSeconds = simSeconds;
        }

        if (!isPermittedTrafficWait(vehicle.phase()) &&
            simSeconds - state->second.lastMovementSeconds > 30.0f) {
            require(false, vehicleDiagnostic(context, seed, count, vehicle,
                                              "watchdog: no movement for over 30 seconds", simSeconds));
        }
    }

    // O(number of cars + number of stalls): a released vehicle retains its
    // destination slotIndex for route geometry, but does not own the Free slot.
    std::vector<int> liveOwner(world.slots().size(), -1);
    for (const auto& vehicle : world.vehicles()) {
        const int slotIndex = vehicle.slotIndex();
        if (slotIndex < 0 || slotIndex >= static_cast<int>(world.slots().size())) continue;
        const ParkingSlot& slot = world.slots()[static_cast<std::size_t>(slotIndex)];
        if (slot.state() == ParkingSlot::State::Free || slot.vehicleId() != vehicle.id()) continue;
        int& owner = liveOwner[static_cast<std::size_t>(slotIndex)];
        if (owner != -1) {
            std::ostringstream message;
            message << context << " seed=" << seed << " N=" << count
                    << " duplicate slot ownership: slot=" << slotIndex
                    << " vehicleA=" << owner << " vehicleB=" << vehicle.id();
            require(false, message.str());
        }
        owner = vehicle.id();
    }
    for (std::size_t slotIndex = 0; slotIndex < world.slots().size(); ++slotIndex) {
        const ParkingSlot& slot = world.slots()[slotIndex];
        if (slot.state() != ParkingSlot::State::Free &&
            liveOwner[slotIndex] != slot.vehicleId()) {
            std::ostringstream message;
            message << context << " seed=" << seed << " N=" << count
                    << " orphan slot=" << slotIndex << " owner=" << slot.vehicleId()
                    << " matchingVehicle=" << liveOwner[slotIndex];
            require(false, message.str());
        }
    }
}

std::uint64_t hashTrafficState(std::uint64_t hash, const ParkingWorld& world) {
    const auto mix = [&hash](std::uint32_t value) {
        hash ^= static_cast<std::uint64_t>(value);
        hash *= 1099511628211ULL;
    };
    const auto mixFloat = [&mix](float value) {
        std::uint32_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value), "32-bit float expected");
        std::memcpy(&bits, &value, sizeof(bits));
        mix(bits);
    };
    mix(static_cast<std::uint32_t>(world.carCount()));
    for (const auto& vehicle : world.vehicles()) {
        mix(static_cast<std::uint32_t>(vehicle.id()));
        mix(static_cast<std::uint32_t>(vehicle.phase()));
        mix(static_cast<std::uint32_t>(vehicle.slotIndex()));
        mixFloat(vehicle.position().x);
        mixFloat(vehicle.position().y);
        mixFloat(vehicle.position().z);
        mixFloat(vehicle.yaw());
    }
    for (const auto& slot : world.slots()) {
        mix(static_cast<std::uint32_t>(slot.state()));
        mix(static_cast<std::uint32_t>(slot.vehicleId()));
    }
    return hash;
}

std::uint64_t runFixedLoadScenario(int count, std::uint32_t seed, float dt,
                                   bool b2 = false) {
    ParkingWorld world(b2 ? AeonLayout::FLOOR_HEIGHT_B2_M : AeonLayout::FLOOR_HEIGHT_B1_M,
                       b2 ? AeonLayout::FLOOR_INDEX_B2 : AeonLayout::FLOOR_INDEX_B1, seed);
    world.initialise();
    world.toggleAutoSpawn();
    world.toggleAutoMotorbikeSpawn();

    int spawned = 0;
    float sinceSpawn = 0.0f;
    std::unordered_map<int, VehicleWatchState> watch;
    std::uint64_t signature = 1469598103934665603ULL;
    const int maxFrames = static_cast<int>(std::ceil(2500.0f / dt));

    for (int frame = 0; frame < maxFrames; ++frame) {
        const float simSeconds = static_cast<float>(frame + 1) * dt;
        sinceSpawn += dt;
        if (spawned < count && sinceSpawn >= 1.5f && world.spawnVehicle()) {
            ++spawned;
            sinceSpawn = 0.0f;
        }

        std::unordered_map<int, glm::vec3> previousPositions;
        previousPositions.reserve(world.vehicles().size());
        for (const auto& vehicle : world.vehicles()) {
            previousPositions.emplace(vehicle.id(), vehicle.position());
        }

        world.update(dt);
        verifyTrafficFrame(world, simSeconds, dt, previousPositions, watch,
                           "fixed-load", static_cast<int>(seed), count);
        signature = hashTrafficState(signature, world);

        if (spawned == count && world.carCount() == 0) {
            require(world.reservedSlotCount() == 0 && world.occupiedSlotCount() == 0 &&
                        world.freeSlotCount() == AeonLayout::TOTAL_ZONE_A_CAR_SLOTS,
                    "fixed-load seed=" + std::to_string(seed) + " N=" +
                        std::to_string(count) + " finished with stale slot state");
            return signature;
        }
    }

    if (!world.vehicles().empty()) {
        const Vehicle& stuck = world.vehicles().front();
        require(false, vehicleDiagnostic("fixed-load", static_cast<int>(seed), count,
                                          stuck, "timeout after 2500 seconds", 2500.0f));
    }
    require(spawned == count, "fixed-load seed=" + std::to_string(seed) + " N=" +
                                  std::to_string(count) + " could not spawn all vehicles");
    require(false, "fixed-load seed=" + std::to_string(seed) + " N=" +
                      std::to_string(count) + " did not clear the parking world");
    return signature;
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


void testMotorbikeArrivalParkingAndDeparture() {
    ParkingWorld world(AeonLayout::FLOOR_HEIGHT_B1_M, AeonLayout::FLOOR_INDEX_B1, 808u);
    world.initialise();
    world.toggleAutoMotorbikeSpawn(); // Keep this scenario deterministic.
    while (world.removeLastMotorbike()) {}
    require(world.motorbikeCount() == 0 && world.freeMotorbikeSlotCount() ==
                AeonLayout::TOTAL_ZONE_A_BIKE_SLOTS,
            "Motorbike test setup should start with an empty annex");
    require(world.spawnMotorbikeTraffic(), "A traffic motorbike should reserve a free bay");
    require(world.motorbikeCount() == 1 && world.freeMotorbikeSlotCount() ==
                AeonLayout::TOTAL_ZONE_A_BIKE_SLOTS - 1,
            "An arriving motorbike must reserve its target bay before parking");

    bool sawAligning = false;
    bool sawEntering = false;
    bool sawParked = false;
    bool sawLeaving = false;
    bool sawReturning = false;
    for (int frame = 0; frame < 6000; ++frame) {
        world.update(0.05f);
        require(world.motorbikeCount() <= 1,
                "The single-lane motorbike AI should serialize moving traffic");
        for (const auto& bike : world.motorbikes()) {
            sawAligning |= bike.phase == ParkedMotorbike::Phase::AligningToBay;
            sawEntering |= bike.phase == ParkedMotorbike::Phase::EnteringBay;
            sawParked |= bike.phase == ParkedMotorbike::Phase::Parked;
            sawLeaving |= bike.phase == ParkedMotorbike::Phase::LeavingBay;
            sawReturning |= bike.phase == ParkedMotorbike::Phase::ReturningToAccess;
        }
        if (world.motorbikeCount() == 0) break;
    }
    require(sawAligning && sawEntering,
            "Motorbike should follow the aisle-alignment and bay-entry waypoints");
    require(sawParked && sawLeaving && sawReturning,
            "Motorbike should park, dwell, leave its bay and return to the access point");
    require(world.motorbikeCount() == 0 &&
                world.freeMotorbikeSlotCount() == AeonLayout::TOTAL_ZONE_A_BIKE_SLOTS,
            "A departed motorbike must be removed and its reserved bay released");
}

void testElevatorMotionAndDoorStateMachine() {
    ElevatorSystem elevator;
    require(elevator.currentFloor() == AeonLayout::ELEVATOR_FLOOR_B1,
            "Elevator should begin at the illustrative B1 landing");
    require(std::abs(elevator.doorOpenAmount() - 1.0f) < 0.001f,
            "Elevator should begin with its doors open");

    bool sawClosing = false;
    bool sawMoving = false;
    bool sawOpening = false;
    bool sawDoorAnimation = false;
    const auto travelTo = [&](int targetFloor) {
        elevator.callToFloor(targetFloor);
        for (int frame = 0; frame < 4000; ++frame) {
            const float previousY = elevator.cabinY();
            elevator.update(0.05f);
            sawClosing |= elevator.phase() == ElevatorSystem::Phase::DoorsClosing;
            sawMoving |= elevator.phase() == ElevatorSystem::Phase::Moving;
            sawOpening |= elevator.phase() == ElevatorSystem::Phase::DoorsOpening;
            sawDoorAnimation |= elevator.doorOpenAmount() > 0.001f &&
                                elevator.doorOpenAmount() < 0.999f;
            if (elevator.phase() == ElevatorSystem::Phase::Moving &&
                std::abs(elevator.cabinY() - previousY) > 0.001f) {
                sawMoving = true;
            }
            if (elevator.currentFloor() == targetFloor &&
                elevator.phase() == ElevatorSystem::Phase::Dwelling &&
                elevator.doorOpenAmount() >= 0.999f) return;
        }
        throw std::runtime_error("Elevator did not complete its requested floor trip");
    };

    travelTo(AeonLayout::ELEVATOR_FLOOR_GROUND);
    require(std::abs(elevator.cabinY() - AeonLayout::ELEVATOR_LEVEL_Y[
                AeonLayout::ELEVATOR_FLOOR_GROUND]) < 0.001f,
            "Elevator cabin did not stop at Ground");
    travelTo(AeonLayout::ELEVATOR_FLOOR_B2);
    require(std::abs(elevator.cabinY() - AeonLayout::ELEVATOR_LEVEL_Y[
                AeonLayout::ELEVATOR_FLOOR_B2]) < 0.001f,
            "Elevator cabin did not stop at B2");
    require(sawClosing && sawMoving && sawOpening && sawDoorAnimation,
            "Elevator did not exercise all door and cabin movement phases");
}

void testHighLoadTrafficEventuallyClearsWithoutDeadlock() {
    ParkingWorld world(AeonLayout::FLOOR_HEIGHT_B1_M, AeonLayout::FLOOR_INDEX_B1, 909u);
    world.initialise();
    world.toggleAutoSpawn(); // Fixed load: no new arrivals after the initial queue.
    for (int i = 0; i < 5; ++i) {
        require(world.spawnVehicle(), "Could not create the five-car stress scenario");
    }

    int peakCars = world.carCount();
    bool observedOccupied = false;
    bool observedPayment = false;
    for (int frame = 0; frame < 30000; ++frame) {
        world.update(0.04f);
        peakCars = std::max(peakCars, world.carCount());
        observedOccupied |= world.occupiedSlotCount() > 0;
        observedPayment |= world.paymentQueueCount() > 0;
        require(!world.hasTrafficCollisions(),
                "Five-car stress scenario produced a vehicle-to-vehicle collision");
        require(!world.hasColumnCollisions(),
                "Five-car stress scenario produced a vehicle-to-column collision");
        if (world.carCount() == 0 && world.reservedSlotCount() == 0 &&
            world.occupiedSlotCount() == 0) break;
    }
    require(peakCars == 5, "Stress scenario did not maintain the intended five-car load");
    require(observedOccupied && observedPayment,
            "Stress scenario did not complete parking and payment states");
    require(world.carCount() == 0 && world.reservedSlotCount() == 0 &&
                world.occupiedSlotCount() == 0,
            "Traffic did not fully clear; possible deadlock or stalled state machine");
    require(world.freeSlotCount() == AeonLayout::TOTAL_ZONE_A_CAR_SLOTS,
            "All car spaces should be released after the stress scenario");
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

void testIntersectionCoverageUnderSustainedArrivals(std::uint32_t seed) {
    ParkingWorld world(AeonLayout::FLOOR_HEIGHT_B1_M, AeonLayout::FLOOR_INDEX_B1, seed);
    world.initialise();
    bool sawSecondAisle = false;
    bool sawPositiveSideRow = false;
    bool sawNegativeSideRow = false;
    bool sawStagingManeuver = false;
    bool sawReverseIntoBay = false;
    bool sawOccupiedState = false;

    // Automatic arrivals exercise both aisles, both row sides, staging, reverse
    // parking and barrier flow. FULL is covered by a separate controlled fixture.
    for (int frame = 0; frame < 25000; ++frame) {
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

    }

    require(sawSecondAisle, "Simulation did not exercise the second aisle");
    require(sawPositiveSideRow && sawNegativeSideRow,
            "Simulation did not exercise both sides of the parking rows");
    require(sawStagingManeuver, "No vehicle reached the staging point before turning into a bay");
    require(sawReverseIntoBay, "No vehicle exercised reverse parking");
    require(sawOccupiedState, "No vehicle reached OCCUPIED state during sustained traffic");
}
void testReportedDeadlockReproducers() {
    // Exact scenarios supplied with the task: A is the five-car same-aisle
    // merge, B is the 64-car FULL-lot exit/merge case.
    (void)runFixedLoadScenario(5, 503u, 0.04f, false);
    (void)runFixedLoadScenario(64, 500u, 0.04f, false);
}

void testLoadMatrixAndDeadlockWatchdog() {
    const int loads[] = {5, 10, 16, 20, 24, 32, 40, 48, 64};
    for (const int count : loads) {
        for (std::uint32_t seed = 500u; seed < 510u; ++seed) {
            (void)runFixedLoadScenario(count, seed, 0.04f, false);
        }
    }
}

void testFullLotCanDrainCompletely() {
    ParkingWorld world(AeonLayout::FLOOR_HEIGHT_B1_M, AeonLayout::FLOOR_INDEX_B1, 501u);
    world.initialise();
    world.toggleAutoSpawn();
    ParkingWorldTestAccess::fillAllStallsAsParked(world);

    require(world.carCount() == AeonLayout::TOTAL_ZONE_A_CAR_SLOTS &&
                world.freeSlotCount() == 0 && world.reservedSlotCount() == 0 &&
                world.occupiedSlotCount() == AeonLayout::TOTAL_ZONE_A_CAR_SLOTS,
            "FULL fixture must start with 64 occupied stalls");
    require(!world.spawnVehicle(), "A 65th car must be rejected while the lot is FULL");
    require(!world.hasTrafficCollisions() && !world.hasColumnCollisions(),
            "FULL fixture must start without vehicle or column overlap");

    std::unordered_map<int, VehicleWatchState> watch;
    for (int frame = 0; frame < 62500 && world.carCount() > 0; ++frame) {
        const float simSeconds = static_cast<float>(frame + 1) * 0.04f;
        std::unordered_map<int, glm::vec3> previous;
        previous.reserve(world.vehicles().size());
        for (const auto& vehicle : world.vehicles())
            previous.emplace(vehicle.id(), vehicle.position());
        world.update(0.04f);
        verifyTrafficFrame(world, simSeconds, 0.04f, previous, watch, "full-lot", 501, 64);
    }

    require(world.carCount() == 0 && world.reservedSlotCount() == 0 &&
                world.occupiedSlotCount() == 0 &&
                world.freeSlotCount() == AeonLayout::TOTAL_ZONE_A_CAR_SLOTS,
            "FULL lot did not drain all 64 vehicles and release every stall");
}

void testMultipleTimeSteps() {
    const float timeSteps[] = {0.016f, 0.04f, 0.1f, 0.25f};
    for (const float dt : timeSteps) {
        (void)runFixedLoadScenario(64, 500u, dt, false);
    }
}

void testDeterministicReplay() {
    const std::uint64_t first = runFixedLoadScenario(64, 777u, 0.04f, false);
    const std::uint64_t second = runFixedLoadScenario(64, 777u, 0.04f, false);
    require(first == second,
            "same seed and dt must produce an identical frame-by-frame traffic signature");
}

void testB2CompleteVehicleLifecycle() {
    ParkingWorld world(AeonLayout::FLOOR_HEIGHT_B2_M, AeonLayout::FLOOR_INDEX_B2, 812u);
    world.initialise();
    world.toggleAutoSpawn();
    world.toggleAutoMotorbikeSpawn();
    for (int index = 0; index < 5; ++index)
        require(world.spawnVehicle(), "B2 lifecycle setup could not spawn five cars");

    bool sawB2ParkingLevel = false;
    bool sawOutboundRampAscent = false;
    std::unordered_map<int, VehicleWatchState> watch;
    for (int frame = 0; frame < 62500 && world.carCount() > 0; ++frame) {
        const float simSeconds = static_cast<float>(frame + 1) * 0.04f;
        std::unordered_map<int, glm::vec3> previous;
        previous.reserve(world.vehicles().size());
        for (const auto& vehicle : world.vehicles()) {
            previous.emplace(vehicle.id(), vehicle.position());
            if (vehicle.phase() == Vehicle::Phase::Parked &&
                std::abs(vehicle.position().y - AeonLayout::FLOOR_HEIGHT_B2_M) < 0.05f)
                sawB2ParkingLevel = true;
            if ((vehicle.phase() == Vehicle::Phase::ExitGateApproach ||
                 vehicle.phase() == Vehicle::Phase::ExitingRamp) &&
                vehicle.position().y > AeonLayout::FLOOR_HEIGHT_B2_M + 0.25f)
                sawOutboundRampAscent = true;
        }
        world.update(0.04f);
        verifyTrafficFrame(world, simSeconds, 0.04f, previous, watch, "B2-lifecycle", 812, 5);
    }

    require(sawB2ParkingLevel, "B2 car did not reach the B2 floor and park");
    require(sawOutboundRampAscent, "B2 car did not climb the outbound ramp");
    require(world.carCount() == 0 && world.reservedSlotCount() == 0 &&
                world.occupiedSlotCount() == 0,
            "B2 traffic did not complete exit and release all stalls");
}

}

int main() {
    try {
        testLayoutAndMotorbikeCapacity();
        testB2GeometryAndIndependentWorld();
        testAnnexAndElevatorGeometryBounds();
        testMotorbikeArrivalParkingAndDeparture();
        testElevatorMotionAndDoorStateMachine();
        testSafeMultiVehicleTrafficAndBarriers();
        testHighLoadTrafficEventuallyClearsWithoutDeadlock();
        // Repeat the sustained-arrival / two-aisle / barrier scenario under
        // fixed seeds to make the regression test reproducible.
        testIntersectionCoverageUnderSustainedArrivals(20261118u);
        testIntersectionCoverageUnderSustainedArrivals(20261119u);
        testIntersectionCoverageUnderSustainedArrivals(20261120u);
        testReportedDeadlockReproducers();
        testLoadMatrixAndDeadlockWatchdog();
        testFullLotCanDrainCompletely();
        testMultipleTimeSteps();
        testDeterministicReplay();
        testB2CompleteVehicleLifecycle();
        std::cout << "All AEON parking world tests passed.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "AEON parking world test failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
