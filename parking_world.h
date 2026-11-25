#pragma once

// ============================================================================
//  parking_world.h - simulation model for a single basement level.
//  Each ParkingWorld instance owns its own cars, motorbike demonstration bays,
//  barriers and floor geometry. B1 and B2 are kept as separate worlds.
// ============================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include <glm/glm.hpp>

#include "aeon_layout.h"

// Player driving request, produced by the application layer.
struct ManualControl {
    float throttle = 0.0f; // +1 = forward, -1 = reverse.
    float steer = 0.0f;    // +1 = left, -1 = right.
};

// One marked car space with its reservation / occupancy state.
class ParkingSlot {
public:
    enum class State { Free, Reserved, Occupied };

    ParkingSlot(const glm::vec3& center, float aisleZ, int rowSide, float facingAisleYaw)
        : center_(center), aisleZ_(aisleZ), rowSide_(rowSide), facingAisleYaw_(facingAisleYaw) {}

    const glm::vec3& center() const { return center_; }
    float aisleZ() const { return aisleZ_; }
    int rowSide() const { return rowSide_; }
    float facingAisleYaw() const { return facingAisleYaw_; }
    State state() const { return state_; }
    int vehicleId() const { return vehicleId_; }

    void reserve(int vehicleId) { state_ = State::Reserved; vehicleId_ = vehicleId; }
    void occupy(int vehicleId) { state_ = State::Occupied; vehicleId_ = vehicleId; }
    void reset() { state_ = State::Free; vehicleId_ = -1; }

private:
    glm::vec3 center_{0.0f};
    float aisleZ_ = 0.0f;
    int rowSide_ = 0;
    float facingAisleYaw_ = 0.0f;
    State state_ = State::Free;
    int vehicleId_ = -1;
};

// The test-only friend is enabled only for the headless test translation unit.
#ifdef AEON_PARKING_WORLD_TEST_ACCESS
struct ParkingWorldTestAccess;
#endif

// One simulated car following the store / park / leave state machine.
class Vehicle {
public:
    enum class Phase {
        EnteringRamp, EntryGateWait, ToEntryConnector, ToTargetAisle, ToBayApproach,
        ToBayStaging, ReversingIntoBay, Parked, LeavingBay, AligningOutboundLane, ToExitConnector,
        ToExitLane, ExitGateApproach, ExitPaymentWait, ExitingRamp
    };

    Vehicle(int id, const glm::vec3& spawnPosition, int slotIndex)
        : id_(id), position_(spawnPosition), slotIndex_(slotIndex) {}

    int id() const { return id_; }
    const glm::vec3& position() const { return position_; }
    float yaw() const { return yaw_; }
    int slotIndex() const { return slotIndex_; }
    Phase phase() const { return phase_; }
    float parkedSeconds() const { return parkedSeconds_; }
    bool isActive() const { return active_; }

private:
    friend class ParkingWorld;
#ifdef AEON_PARKING_WORLD_TEST_ACCESS
    friend struct ParkingWorldTestAccess;
#endif

    int id_ = -1;
    glm::vec3 position_{0.0f};
    float yaw_ = 0.0f;
    int slotIndex_ = -1;
    Phase phase_ = Phase::EnteringRamp;
    float phaseTimer_ = 0.0f;
    float parkedSeconds_ = 0.0f;
    bool active_ = true;
    // Once a departure has been admitted, do not revoke its right-of-way in
    // the middle of the aisle manoeuvre when a new inbound car appears.
    bool departureManeuverStarted_ = false;
    // Exclusive right-of-way while crossing into the shared exit lane.
    bool exitMergeStarted_ = false;
    // Connector-crossing reservation acquired before entering the shared
    // entry/exit conflict envelope and released after reaching the inbound aisle.
    bool entryConnectorStarted_ = false;
    bool connectorQueueWait_ = false;
    bool rampQueueWait_ = false;
};

struct ParkedMotorbike {
    enum class Phase { Parked, AligningToBay, EnteringBay, LeavingBay, ReturningToAccess, Removed };
    glm::vec3 position{0.0f};
    float yaw = 0.0f;
    int slotIndex = -1;
    Phase phase = Phase::Parked;
    float dwellTimer = 0.0f;
};

// ---------------------------------------------------------------------------
// Simple two-shaft elevator controller (shared across B1/B2 views).
// This is an illustrative animation only; it does not model real AEON lift
// control, capacity or passenger flow. All timings and cabin sizes are
// marked [ASSUMED] in aeon_layout.h.
// ---------------------------------------------------------------------------
class ElevatorSystem {
public:
    enum class Phase {
        Idle,
        DoorsClosing,
        Moving,
        DoorsOpening,
        Dwelling
    };

    ElevatorSystem() {
        cabinY_ = AeonLayout::ELEVATOR_LEVEL_Y[AeonLayout::ELEVATOR_FLOOR_B1];
        currentFloor_ = AeonLayout::ELEVATOR_FLOOR_B1;
        targetFloor_ = currentFloor_;
        doorOpenAmount_ = 1.0f; // start open at B1 for visibility
        phase_ = Phase::Dwelling;
        phaseTimer_ = AeonLayout::ELEVATOR_DWELL_OPEN_DURATION_S;
    }

    void update(float dt) {
        phaseTimer_ -= dt;
        switch (phase_) {
        case Phase::Idle:
            // Auto-cycle through floors so the animation is always visible.
            if (phaseTimer_ <= 0.0f) {
                int next = (currentFloor_ + 1) % 3;
                callToFloor(next);
            }
            break;
        case Phase::DoorsClosing:
            doorOpenAmount_ = std::max(0.0f, doorOpenAmount_ -
                dt / AeonLayout::ELEVATOR_DOOR_CLOSE_DURATION_S);
            if (doorOpenAmount_ <= 0.0f) {
                doorOpenAmount_ = 0.0f;
                phase_ = Phase::Moving;
                // travel time based on distance
                const float dist = std::abs(AeonLayout::ELEVATOR_LEVEL_Y[targetFloor_] - cabinY_);
                phaseTimer_ = dist / AeonLayout::ELEVATOR_TRAVEL_SPEED_M_S;
            }
            break;
        case Phase::Moving: {
            const float targetY = AeonLayout::ELEVATOR_LEVEL_Y[targetFloor_];
            const float dir = (targetY > cabinY_) ? 1.0f : -1.0f;
            cabinY_ += dir * AeonLayout::ELEVATOR_TRAVEL_SPEED_M_S * dt;
            if ((dir > 0.0f && cabinY_ >= targetY) || (dir < 0.0f && cabinY_ <= targetY)) {
                cabinY_ = targetY;
                currentFloor_ = targetFloor_;
                phase_ = Phase::DoorsOpening;
                phaseTimer_ = AeonLayout::ELEVATOR_DOOR_OPEN_DURATION_S;
            }
            break;
        }
        case Phase::DoorsOpening:
            doorOpenAmount_ = std::min(1.0f, doorOpenAmount_ +
                dt / AeonLayout::ELEVATOR_DOOR_OPEN_DURATION_S);
            if (doorOpenAmount_ >= 1.0f) {
                doorOpenAmount_ = 1.0f;
                phase_ = Phase::Dwelling;
                phaseTimer_ = AeonLayout::ELEVATOR_DWELL_OPEN_DURATION_S;
            }
            break;
        case Phase::Dwelling:
            if (phaseTimer_ <= 0.0f) {
                phase_ = Phase::Idle;
                phaseTimer_ = AeonLayout::ELEVATOR_IDLE_RECALL_DELAY_S;
            }
            break;
        }
    }

    // Request the car to a floor (0=G, 1=B1, 2=B2).
    void callToFloor(int floor) {
        if (floor < 0 || floor > 2) return;
        if (phase_ == Phase::Moving || phase_ == Phase::DoorsClosing) return; // busy
        if (floor == currentFloor_ && phase_ != Phase::Moving) {
            // already here – just open doors
            phase_ = Phase::DoorsOpening;
            phaseTimer_ = AeonLayout::ELEVATOR_DOOR_OPEN_DURATION_S;
            return;
        }
        targetFloor_ = floor;
        if (doorOpenAmount_ > 0.01f) {
            phase_ = Phase::DoorsClosing;
            phaseTimer_ = AeonLayout::ELEVATOR_DOOR_CLOSE_DURATION_S;
        } else {
            phase_ = Phase::Moving;
            const float dist = std::abs(AeonLayout::ELEVATOR_LEVEL_Y[targetFloor_] - cabinY_);
            phaseTimer_ = dist / AeonLayout::ELEVATOR_TRAVEL_SPEED_M_S;
        }
    }

    float cabinY() const { return cabinY_; }
    float doorOpenAmount() const { return doorOpenAmount_; } // 0 = closed, 1 = fully open
    int currentFloor() const { return currentFloor_; }
    int targetFloor() const { return targetFloor_; }
    Phase phase() const { return phase_; }
    const char* phaseName() const {
        switch (phase_) {
        case Phase::Idle: return "IDLE";
        case Phase::DoorsClosing: return "CLOSING";
        case Phase::Moving: return "MOVING";
        case Phase::DoorsOpening: return "OPENING";
        case Phase::Dwelling: return "DWELL";
        }
        return "?";
    }

private:
    float cabinY_ = 0.0f;
    float doorOpenAmount_ = 0.0f;
    int currentFloor_ = AeonLayout::ELEVATOR_FLOOR_B1;
    int targetFloor_ = AeonLayout::ELEVATOR_FLOOR_B1;
    Phase phase_ = Phase::Idle;
    float phaseTimer_ = 0.0f;
};

class ParkingWorld {
public:
    explicit ParkingWorld(float floorY = AeonLayout::FLOOR_HEIGHT_B1_M,
                          int floorIndex = AeonLayout::FLOOR_INDEX_B1,
                          std::uint32_t randomSeed = std::random_device{}())
        : floorY_(floorY),
          upperFloorY_(floorIndex == AeonLayout::FLOOR_INDEX_B2
              ? AeonLayout::B2_UPPER_LEVEL_Y_M : AeonLayout::GROUND_LEVEL_Y_M),
          floorIndex_(floorIndex),
          playerPosition_(AeonLayout::ZONE_MAX_X_M - AeonLayout::PERIMETER_LANE_W_M * AeonLayout::GEOMETRY_HALF_RATIO,
                          floorY,
                          AeonLayout::ZONE_MAX_Z_M - AeonLayout::PERIMETER_LANE_W_M * AeonLayout::GEOMETRY_HALF_RATIO),
          randomEngine_(randomSeed) {}

    void initialise();

    // Simulation step: advance every vehicle, then release the finished ones.
    void update(float dt);
    void updateSpawning(float dt);

    bool spawnVehicle();
    void spawnNow();
    bool spawnMotorbike(); // Add a stationary demonstration bike (used for initial population/tests).
    bool spawnMotorbikeTraffic(); // Spawn a bike that drives into a free bay using waypoints.
    bool removeLastMotorbike();
    void toggleAutoMotorbikeSpawn() { autoMotorbikeSpawn_ = !autoMotorbikeSpawn_; }
    bool autoMotorbikeSpawn() const { return autoMotorbikeSpawn_; }
    int movingMotorbikeCount() const;
    void toggleAutoSpawn() { autoSpawn_ = !autoSpawn_; }
    bool autoSpawn() const { return autoSpawn_; }

    void toggleManualMode() { manualMode_ = !manualMode_; }
    bool manualMode() const { return manualMode_; }
    void updateManualCar(const ManualControl& control, float dt);

    const std::vector<ParkingSlot>& slots() const { return slots_; }
    const std::vector<Vehicle>& vehicles() const { return traffic_; }
    const std::vector<glm::vec3>& columns() const { return columns_; }
    const std::vector<ParkedMotorbike>& motorbikes() const { return motorbikes_; }
    const std::vector<glm::vec3>& motorbikeSlots() const { return motorbikeSlotCenters_; }
    float floorY() const { return floorY_; }
    float upperFloorY() const { return upperFloorY_; }
    int floorIndex() const { return floorIndex_; }
    bool hasTrafficCollisions() const;
    bool hasColumnCollisions() const;

    int carCount() const { return static_cast<int>(traffic_.size()); }
    int motorbikeCount() const { return static_cast<int>(motorbikes_.size()); }
    int freeMotorbikeSlotCount() const;
    int freeSlotCount() const;
    int reservedSlotCount() const;
    int occupiedSlotCount() const;
    int paymentQueueCount() const;

    float entryBarrierOpenTimer() const { return entryBarrierTimer_; }
    float exitBarrierOpenTimer() const { return exitBarrierTimer_; }

    const glm::vec3& playerPosition() const { return playerPosition_; }
    float playerYaw() const { return playerYaw_; }

private:
#ifdef AEON_PARKING_WORLD_TEST_ACCESS
    friend struct ParkingWorldTestAccess;
#endif

    void updateVehicle(Vehicle& vehicle, float dt);
    void updateMotorbikes(float dt);
    void releaseSlot(const Vehicle& vehicle);
    bool rotateToward(Vehicle& vehicle, float targetYaw, float dt);
    bool moveMotorbikeToward(ParkedMotorbike& bike, const glm::vec3& target, float dt);
    bool moveToward(Vehicle& vehicle, const glm::vec3& target, float speed, float dt,
                    bool reverse = false, bool followRamp = false);
    bool isBlockedByTraffic(int ignoreVehicleId, const glm::vec3& candidate, float yaw) const;
    bool isBlockedByStaticObjects(const glm::vec3& candidate, float yaw) const;
    bool hasInboundManeuverOnAisle(const Vehicle& departing) const;
    bool hasActiveDepartureOnAisle(const Vehicle& departing) const;
    bool hasActiveExitMerge(const Vehicle& merging) const;
    bool hasActiveEntryConnector(const Vehicle& crossing) const;
    bool hasActiveDepartureAtConnector(const Vehicle& crossing) const;
    bool overlapsFootprints(const glm::vec3& aPosition, float aYaw,
                            const glm::vec3& bPosition, float bYaw) const;
    static void vehicleHalfExtents(float yaw, float& halfX, float& halfZ);
    int findFreeSlot() const;

    // [ASSUMED] Low-level kinematic tuning for the rule-based vehicle model.
    static constexpr float AUTO_TURN_RATE_RAD_S = 2.0f;
    static constexpr float ROTATION_COLLISION_SUBSTEP_RAD = 0.05f;
    static constexpr float OUTBOUND_YAW_TOLERANCE_RAD = 0.01f;
    // [DERIVED] Keep a full car-length plus two safety gaps before merging
    // laterally into the paired exit lane, so a following outbound car clears.
    static constexpr float EXIT_LANE_CLEARANCE_X_M =
        AeonLayout::CAR_LENGTH_M + 2.0f * AeonLayout::VEHICLE_SAFETY_GAP_M;

    std::vector<ParkingSlot> slots_;
    std::vector<Vehicle> traffic_;
    std::vector<glm::vec3> columns_;
    std::vector<glm::vec3> motorbikeSlotCenters_;
    std::vector<bool> motorbikeSlotOccupied_;
    std::vector<ParkedMotorbike> motorbikes_;
    float floorY_ = AeonLayout::FLOOR_HEIGHT_B1_M;
    float upperFloorY_ = AeonLayout::GROUND_LEVEL_Y_M;
    int floorIndex_ = AeonLayout::FLOOR_INDEX_B1;

    int nextVehicleId_ = 1;
    float entryBarrierTimer_ = 0.0f;
    float exitBarrierTimer_ = 0.0f;
    bool autoSpawn_ = true;
    float spawnTimer_ = 0.0f;
    float motorbikeSpawnTimer_ = 0.0f;
    bool autoMotorbikeSpawn_ = true;

    bool manualMode_ = false;
    glm::vec3 playerPosition_{0.0f};
    float playerYaw_ = AeonLayout::PI;

    std::mt19937 randomEngine_;
    std::uniform_real_distribution<float> dwellDistribution_{
        AeonLayout::PARKING_DWELL_MIN_SECONDS, AeonLayout::PARKING_DWELL_MAX_SECONDS};
    std::uniform_real_distribution<float> motorbikeDwellDistribution_{
        AeonLayout::BIKE_AI_DWELL_MIN_S, AeonLayout::BIKE_AI_DWELL_MAX_S};
};

// ------------------------------------------------------------------ layout
inline void ParkingWorld::initialise() {
    slots_.clear();
    columns_.clear();
    traffic_.clear();
    motorbikeSlotCenters_.clear();
    motorbikeSlotOccupied_.clear();
    motorbikes_.clear();
    nextVehicleId_ = 1;
    entryBarrierTimer_ = exitBarrierTimer_ = spawnTimer_ = motorbikeSpawnTimer_ = 0.0f;
    playerPosition_ = glm::vec3(
        AeonLayout::ZONE_MAX_X_M - AeonLayout::PERIMETER_LANE_W_M * AeonLayout::GEOMETRY_HALF_RATIO,
        floorY_,
        AeonLayout::ZONE_MAX_Z_M - AeonLayout::PERIMETER_LANE_W_M * AeonLayout::GEOMETRY_HALF_RATIO);

    for (const float aisleZ : AeonLayout::AISLE_CENTER_Z_M) {
        for (const int rowSide : {-1, 1}) {
            const float rowZ = AeonLayout::slotCenterZ(aisleZ, rowSide);
            const float facingAisleYaw =
                rowSide > 0 ? AeonLayout::PI * AeonLayout::GEOMETRY_HALF_RATIO : -AeonLayout::PI * AeonLayout::GEOMETRY_HALF_RATIO;
            for (int slotIndex = 0; slotIndex < AeonLayout::STALLS_PER_ROW; ++slotIndex) {
                slots_.emplace_back(
                    glm::vec3(AeonLayout::vehicleBayCenterX(slotIndex), floorY_, rowZ),
                    aisleZ, rowSide, facingAisleYaw);
            }
        }
    }

    // Five full 7.5 m column spans are generated along each aisle edge.
    // The remaining 2.5 m at the east end is documented in SURVEY_AEON.md.
    for (const float aisleZ : AeonLayout::AISLE_CENTER_Z_M) {
        for (const int edgeSide : {-1, 1}) {
            const float z = aisleZ + static_cast<float>(edgeSide) * AeonLayout::AISLE_W_M * AeonLayout::GEOMETRY_HALF_RATIO;
            for (int grid = 1; grid <= AeonLayout::COLUMN_GRID_X_COUNT; ++grid) {
                const float x = AeonLayout::CORE_MIN_X_M +
                                static_cast<float>(grid) * AeonLayout::COLUMN_SPACING_X_M;
                columns_.emplace_back(x, floorY_, z);
            }
        }
    }

    // Separate 24-space motorbike demo bay. This annex is an explicit design
    // assumption, outside the 52 x 44 m car Zone A footprint.
    for (int row = 0; row < AeonLayout::BIKE_ROW_COUNT; ++row) {
        for (int index = 0; index < AeonLayout::BIKE_STALLS_PER_ROW; ++index) {
            motorbikeSlotCenters_.emplace_back(
                AeonLayout::motorbikeSlotX(index), floorY_, AeonLayout::motorbikeSlotZ(row));
            motorbikeSlotOccupied_.push_back(false);
        }
    }
    for (int index = 0; index < AeonLayout::INITIAL_PARKED_MOTORBIKES; ++index) {
        spawnMotorbike();
    }
}

// ------------------------------------------------------------------- counts
inline int ParkingWorld::freeSlotCount() const {
    return static_cast<int>(std::count_if(slots_.begin(), slots_.end(),
        [](const ParkingSlot& slot) { return slot.state() == ParkingSlot::State::Free; }));
}

inline int ParkingWorld::reservedSlotCount() const {
    return static_cast<int>(std::count_if(slots_.begin(), slots_.end(),
        [](const ParkingSlot& slot) { return slot.state() == ParkingSlot::State::Reserved; }));
}

inline int ParkingWorld::occupiedSlotCount() const {
    return static_cast<int>(std::count_if(slots_.begin(), slots_.end(),
        [](const ParkingSlot& slot) { return slot.state() == ParkingSlot::State::Occupied; }));
}

inline int ParkingWorld::paymentQueueCount() const {
    return static_cast<int>(std::count_if(traffic_.begin(), traffic_.end(),
        [](const Vehicle& vehicle) { return vehicle.phase() == Vehicle::Phase::ExitPaymentWait; }));
}

inline int ParkingWorld::freeMotorbikeSlotCount() const {
    return static_cast<int>(std::count(motorbikeSlotOccupied_.begin(),
                                       motorbikeSlotOccupied_.end(), false));
}

inline int ParkingWorld::movingMotorbikeCount() const {
    return static_cast<int>(std::count_if(motorbikes_.begin(), motorbikes_.end(),
        [](const ParkedMotorbike& bike) { return bike.phase != ParkedMotorbike::Phase::Parked; }));
}

inline bool ParkingWorld::hasTrafficCollisions() const {
    struct HalfExtents { float x; float z; };
    std::array<HalfExtents, AeonLayout::MAX_ACTIVE_AUTO_VEHICLES> extents{};
    const std::size_t count = std::min(traffic_.size(), extents.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (traffic_[i].isActive()) {
            vehicleHalfExtents(traffic_[i].yaw(), extents[i].x, extents[i].z);
        }
    }

    const float gap = AeonLayout::VEHICLE_SAFETY_GAP_M;
    const float verticalTolerance = AeonLayout::RAMP_COLLISION_VERTICAL_TOLERANCE_M;
    float playerHalfX = 0.0f;
    float playerHalfZ = 0.0f;
    if (manualMode_) vehicleHalfExtents(playerYaw_, playerHalfX, playerHalfZ);

    for (std::size_t i = 0; i < count; ++i) {
        const Vehicle& a = traffic_[i];
        if (!a.isActive()) continue;
        for (std::size_t j = i + 1; j < count; ++j) {
            const Vehicle& b = traffic_[j];
            if (!b.isActive() || std::abs(a.position().y - b.position().y) > verticalTolerance)
                continue;
            if (std::abs(a.position().x - b.position().x) < extents[i].x + extents[j].x + gap &&
                std::abs(a.position().z - b.position().z) < extents[i].z + extents[j].z + gap) {
                return true;
            }
        }
        if (manualMode_ &&
            std::abs(a.position().y - playerPosition_.y) <= verticalTolerance &&
            std::abs(a.position().x - playerPosition_.x) < extents[i].x + playerHalfX + gap &&
            std::abs(a.position().z - playerPosition_.z) < extents[i].z + playerHalfZ + gap) {
            return true;
        }
    }
    return false;
}

inline bool ParkingWorld::hasColumnCollisions() const {
    for (const auto& vehicle : traffic_) {
        if (!vehicle.isActive()) continue;
        if (isBlockedByStaticObjects(vehicle.position(), vehicle.yaw())) return true;
    }
    return false;
}

inline int ParkingWorld::findFreeSlot() const {
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        if (slots_[i].state() == ParkingSlot::State::Free) return static_cast<int>(i);
    }
    return -1;
}

// ------------------------------------------------------------------ spawning
inline bool ParkingWorld::spawnVehicle() {
    const int slotIndex = findFreeSlot();
    if (slotIndex < 0 ||
        static_cast<int>(traffic_.size()) >= AeonLayout::MAX_ACTIVE_AUTO_VEHICLES) {
        return false;
    }

    // Queue arrivals at safe intervals behind the ramp mouth. This allows
    // multiple vehicles to be spawned for repeatable traffic/intersection tests.
    const float queueSpacing = AeonLayout::CAR_LENGTH_M + AeonLayout::VEHICLE_SAFETY_GAP_M;
    const int queueCapacity = static_cast<int>(AeonLayout::RAMP_LENGTH_X_M / queueSpacing) +
                              AeonLayout::RAMP_QUEUE_EXTRA_POSITIONS;
    glm::vec3 spawnPosition(AeonLayout::RAMP_START_X_M, upperFloorY_,
                            AeonLayout::ENTRY_LANE_CENTER_Z_M);
    bool foundPosition = false;
    for (int queueIndex = 0; queueIndex < queueCapacity; ++queueIndex) {
        const glm::vec3 candidate(AeonLayout::RAMP_START_X_M -
                                      static_cast<float>(queueIndex) * queueSpacing,
                                  upperFloorY_, AeonLayout::ENTRY_LANE_CENTER_Z_M);
        bool blocked = false;
        for (const auto& vehicle : traffic_) {
            if (overlapsFootprints(candidate, 0.0f, vehicle.position(), vehicle.yaw())) {
                blocked = true;
                break;
            }
        }
        if (!blocked) {
            spawnPosition = candidate;
            foundPosition = true;
            break;
        }
    }
    if (!foundPosition) return false;

    Vehicle vehicle(nextVehicleId_++, spawnPosition, slotIndex);
    slots_[static_cast<std::size_t>(slotIndex)].reserve(vehicle.id());
    traffic_.push_back(vehicle);
    return true;
}

inline bool ParkingWorld::spawnMotorbike() {
    for (std::size_t index = 0; index < motorbikeSlotOccupied_.size(); ++index) {
        if (motorbikeSlotOccupied_[index]) continue;
        motorbikeSlotOccupied_[index] = true;
        const int row = static_cast<int>(index) / AeonLayout::BIKE_STALLS_PER_ROW;
        const float yaw = (row == 0) ? AeonLayout::PI : 0.0f;
        ParkedMotorbike bike;
        bike.position = motorbikeSlotCenters_[index];
        bike.yaw = yaw;
        bike.slotIndex = static_cast<int>(index);
        bike.phase = ParkedMotorbike::Phase::Parked;
        bike.dwellTimer = motorbikeDwellDistribution_(randomEngine_);
        motorbikes_.push_back(bike);
        return true;
    }
    return false;
}

inline bool ParkingWorld::spawnMotorbikeTraffic() {
    // One moving motorbike at a time keeps the narrow illustrative aisle clear.
    if (movingMotorbikeCount() > 0) return false;
    for (std::size_t index = 0; index < motorbikeSlotOccupied_.size(); ++index) {
        if (motorbikeSlotOccupied_[index]) continue;
        motorbikeSlotOccupied_[index] = true; // Reserve before the bike reaches the bay.
        ParkedMotorbike bike;
        bike.position = glm::vec3(AeonLayout::BIKE_ZONE_CENTER_X_M, floorY_,
                                  AeonLayout::BIKE_ZONE_CENTER_Z_M);
        bike.yaw = AeonLayout::PI * AeonLayout::GEOMETRY_HALF_RATIO;
        bike.slotIndex = static_cast<int>(index);
        bike.phase = ParkedMotorbike::Phase::AligningToBay;
        bike.dwellTimer = 0.0f;
        motorbikes_.push_back(bike);
        motorbikeSpawnTimer_ = 0.0f;
        return true;
    }
    return false;
}

inline bool ParkingWorld::removeLastMotorbike() {
    if (motorbikes_.empty()) return false;
    const int slotIndex = motorbikes_.back().slotIndex;
    if (slotIndex >= 0 && slotIndex < static_cast<int>(motorbikeSlotOccupied_.size())) {
        motorbikeSlotOccupied_[static_cast<std::size_t>(slotIndex)] = false;
    }
    motorbikes_.pop_back();
    return true;
}

inline bool ParkingWorld::moveMotorbikeToward(ParkedMotorbike& bike,
                                               const glm::vec3& target, float dt) {
    const float dx = target.x - bike.position.x;
    const float dz = target.z - bike.position.z;
    const float distance = std::sqrt(dx * dx + dz * dz);
    if (distance <= AeonLayout::BIKE_AI_ARRIVAL_EPSILON_M) {
        bike.position = target;
        return true;
    }
    const float step = std::min(AeonLayout::BIKE_AI_SPEED_MPS * dt, distance);
    const float invDistance = 1.0f / distance;
    const float dirX = dx * invDistance;
    const float dirZ = dz * invDistance;
    bike.position.x += dirX * step;
    bike.position.z += dirZ * step;
    bike.position.y = floorY_;
    // Bike model's local forward axis is +Z, so yaw is measured from +Z.
    bike.yaw = std::atan2(dirX, dirZ);
    return distance <= step + AeonLayout::BIKE_AI_ARRIVAL_EPSILON_M;
}

inline void ParkingWorld::updateMotorbikes(float dt) {
    if (dt <= 0.0f) return;

    bool movingBikePresent = false;
    for (auto& bike : motorbikes_) {
        if (bike.phase == ParkedMotorbike::Phase::Parked) {
            bike.dwellTimer = std::max(0.0f, bike.dwellTimer - dt);
            continue;
        }
        movingBikePresent = true;
        if (bike.slotIndex < 0 || bike.slotIndex >= static_cast<int>(motorbikeSlotCenters_.size())) {
            bike.phase = ParkedMotorbike::Phase::Removed;
            continue;
        }
        const glm::vec3 aislePoint(motorbikeSlotCenters_[static_cast<std::size_t>(bike.slotIndex)].x,
                                   floorY_, AeonLayout::BIKE_ZONE_CENTER_Z_M);
        switch (bike.phase) {
        case ParkedMotorbike::Phase::Parked:
            break;
        case ParkedMotorbike::Phase::AligningToBay: {
            if (moveMotorbikeToward(bike, aislePoint, dt)) {
                bike.phase = ParkedMotorbike::Phase::EnteringBay;
            }
            break;
        }
        case ParkedMotorbike::Phase::EnteringBay: {
            const glm::vec3 bay = motorbikeSlotCenters_[static_cast<std::size_t>(bike.slotIndex)];
            if (moveMotorbikeToward(bike, bay, dt)) {
                const int row = bike.slotIndex / AeonLayout::BIKE_STALLS_PER_ROW;
                bike.yaw = row == 0 ? AeonLayout::PI : 0.0f;
                bike.phase = ParkedMotorbike::Phase::Parked;
                bike.dwellTimer = motorbikeDwellDistribution_(randomEngine_);
            }
            break;
        }
        case ParkedMotorbike::Phase::LeavingBay:
            if (moveMotorbikeToward(bike, aislePoint, dt)) {
                bike.phase = ParkedMotorbike::Phase::ReturningToAccess;
            }
            break;
        case ParkedMotorbike::Phase::ReturningToAccess: {
            const glm::vec3 accessPoint(AeonLayout::BIKE_ZONE_CENTER_X_M, floorY_,
                                        AeonLayout::BIKE_ZONE_CENTER_Z_M);
            if (moveMotorbikeToward(bike, accessPoint, dt)) {
                bike.phase = ParkedMotorbike::Phase::Removed;
            }
            break;
        }
        case ParkedMotorbike::Phase::Removed:
            break;
        }
    }

    // Only one bike uses the assumed narrow manoeuvring aisle at a time.
    // Expired parked bikes wait until the aisle becomes free.
    movingBikePresent = movingMotorbikeCount() > 0;
    if (!movingBikePresent) {
        for (auto& bike : motorbikes_) {
            if (bike.phase == ParkedMotorbike::Phase::Parked && bike.dwellTimer <= 0.0f) {
                bike.phase = ParkedMotorbike::Phase::LeavingBay;
                break;
            }
        }
    }

    for (auto it = motorbikes_.begin(); it != motorbikes_.end();) {
        if (it->phase == ParkedMotorbike::Phase::Removed) {
            const int slotIndex = it->slotIndex;
            if (slotIndex >= 0 && slotIndex < static_cast<int>(motorbikeSlotOccupied_.size()))
                motorbikeSlotOccupied_[static_cast<std::size_t>(slotIndex)] = false;
            it = motorbikes_.erase(it);
        } else {
            ++it;
        }
    }

    motorbikeSpawnTimer_ += dt;
    if (autoMotorbikeSpawn_ && movingMotorbikeCount() == 0 &&
        freeMotorbikeSlotCount() > 0 &&
        motorbikeSpawnTimer_ >= AeonLayout::BIKE_AI_SPAWN_INTERVAL_S) {
        spawnMotorbikeTraffic();
    }
}

inline void ParkingWorld::spawnNow() {
    if (spawnVehicle()) spawnTimer_ = 0.0f;
}

inline void ParkingWorld::updateSpawning(float dt) {
    if (!autoSpawn_) return;
    spawnTimer_ += dt;
    if (spawnTimer_ >= AeonLayout::SPAWN_INTERVAL_SECONDS &&
        static_cast<int>(traffic_.size()) < AeonLayout::MAX_ACTIVE_AUTO_VEHICLES) {
        if (spawnVehicle()) spawnTimer_ = 0.0f;
        else spawnTimer_ = AeonLayout::SPAWN_INTERVAL_SECONDS * 0.75f;
    }
}

inline void ParkingWorld::releaseSlot(const Vehicle& vehicle) {
    if (vehicle.slotIndex_ < 0 ||
        vehicle.slotIndex_ >= static_cast<int>(slots_.size())) {
        return;
    }
    ParkingSlot& slot = slots_[static_cast<std::size_t>(vehicle.slotIndex_)];
    if (slot.vehicleId() == vehicle.id_) {
        slot.reset();
    }
}

// --------------------------------------------------------------- geometry
inline void ParkingWorld::vehicleHalfExtents(float yaw, float& halfX, float& halfZ) {
    const float c = std::abs(std::cos(yaw));
    const float s = std::abs(std::sin(yaw));
    halfX = c * AeonLayout::CAR_LENGTH_M * AeonLayout::GEOMETRY_HALF_RATIO + s * AeonLayout::CAR_WIDTH_M * AeonLayout::GEOMETRY_HALF_RATIO;
    halfZ = s * AeonLayout::CAR_LENGTH_M * AeonLayout::GEOMETRY_HALF_RATIO + c * AeonLayout::CAR_WIDTH_M * AeonLayout::GEOMETRY_HALF_RATIO;
}

inline bool ParkingWorld::overlapsFootprints(const glm::vec3& aPosition, float aYaw,
                                             const glm::vec3& bPosition, float bYaw) const {
    if (std::abs(aPosition.y - bPosition.y) > AeonLayout::RAMP_COLLISION_VERTICAL_TOLERANCE_M) {
        return false;
    }
    float aHalfX = 0.0f, aHalfZ = 0.0f, bHalfX = 0.0f, bHalfZ = 0.0f;
    vehicleHalfExtents(aYaw, aHalfX, aHalfZ);
    vehicleHalfExtents(bYaw, bHalfX, bHalfZ);
    return std::abs(aPosition.x - bPosition.x) <
               aHalfX + bHalfX + AeonLayout::VEHICLE_SAFETY_GAP_M &&
           std::abs(aPosition.z - bPosition.z) <
               aHalfZ + bHalfZ + AeonLayout::VEHICLE_SAFETY_GAP_M;
}

inline bool ParkingWorld::isBlockedByTraffic(int ignoreVehicleId, const glm::vec3& candidate,
                                             float yaw) const {
    float candidateHalfX = 0.0f;
    float candidateHalfZ = 0.0f;
    vehicleHalfExtents(yaw, candidateHalfX, candidateHalfZ);
    const float gap = AeonLayout::VEHICLE_SAFETY_GAP_M;
    const float verticalTolerance = AeonLayout::RAMP_COLLISION_VERTICAL_TOLERANCE_M;

    for (const auto& other : traffic_) {
        if (other.id_ == ignoreVehicleId || !other.active_ ||
            std::abs(candidate.y - other.position_.y) > verticalTolerance) continue;
        float otherHalfX = 0.0f;
        float otherHalfZ = 0.0f;
        vehicleHalfExtents(other.yaw_, otherHalfX, otherHalfZ);
        if (std::abs(candidate.x - other.position_.x) < candidateHalfX + otherHalfX + gap &&
            std::abs(candidate.z - other.position_.z) < candidateHalfZ + otherHalfZ + gap) {
            return true;
        }
    }
    if (manualMode_ && overlapsFootprints(candidate, yaw, playerPosition_, playerYaw_)) {
        return true;
    }
    return false;
}

inline bool ParkingWorld::hasInboundManeuverOnAisle(const Vehicle& departing) const {
    if (departing.slotIndex_ < 0 ||
        departing.slotIndex_ >= static_cast<int>(slots_.size())) return false;
    const float aisleZ = slots_[static_cast<std::size_t>(departing.slotIndex_)].aisleZ();
    for (const auto& other : traffic_) {
        if (!other.active_ || other.id_ == departing.id_ || other.slotIndex_ < 0 ||
            other.slotIndex_ >= static_cast<int>(slots_.size())) continue;
        if (std::abs(slots_[static_cast<std::size_t>(other.slotIndex_)].aisleZ() - aisleZ) >
            AeonLayout::ROUTE_ARRIVAL_EPSILON_M) continue;
        if (other.entryConnectorStarted_ ||
            other.phase_ == Vehicle::Phase::ToBayApproach ||
            other.phase_ == Vehicle::Phase::ToBayStaging ||
            other.phase_ == Vehicle::Phase::ReversingIntoBay) return true;
    }
    return false;
}

inline bool ParkingWorld::hasActiveDepartureOnAisle(const Vehicle& departing) const {
    if (departing.slotIndex_ < 0 ||
        departing.slotIndex_ >= static_cast<int>(slots_.size())) return false;
    const float aisleZ = slots_[static_cast<std::size_t>(departing.slotIndex_)].aisleZ();
    for (const auto& other : traffic_) {
        if (!other.active_ || other.id_ == departing.id_ || other.slotIndex_ < 0 ||
            other.slotIndex_ >= static_cast<int>(slots_.size())) continue;
        if (std::abs(slots_[static_cast<std::size_t>(other.slotIndex_)].aisleZ() - aisleZ) >
            AeonLayout::ROUTE_ARRIVAL_EPSILON_M) continue;
        // Keep the aisle reservation until the older vehicle reaches the
        // west connector. Releasing the turn right after lane alignment lets
        // the next car enter the same manoeuvring envelope too early.
        if (other.departureManeuverStarted_ &&
            (other.phase_ == Vehicle::Phase::LeavingBay ||
             other.phase_ == Vehicle::Phase::AligningOutboundLane ||
             other.phase_ == Vehicle::Phase::ToExitConnector ||
             other.phase_ == Vehicle::Phase::ToExitLane)) return true;
    }
    return false;
}

inline bool ParkingWorld::hasActiveExitMerge(const Vehicle& merging) const {
    for (const auto& other : traffic_) {
        if (other.active_ && other.id_ != merging.id_ && other.exitMergeStarted_) {
            return true;
        }
    }
    return false;
}

inline bool ParkingWorld::hasActiveEntryConnector(const Vehicle& crossing) const {
    for (const auto& other : traffic_) {
        if (other.active_ && other.id_ != crossing.id_ && other.entryConnectorStarted_) {
            return true;
        }
    }
    return false;
}

inline bool ParkingWorld::hasActiveDepartureAtConnector(const Vehicle& crossing) const {
    for (const auto& other : traffic_) {
        if (!other.active_ || other.id_ == crossing.id_) continue;
        if (other.exitMergeStarted_ ||
            (other.departureManeuverStarted_ &&
             (other.phase_ == Vehicle::Phase::ToExitConnector ||
              other.phase_ == Vehicle::Phase::ToExitLane))) {
            return true;
        }
    }
    return false;
}

inline bool ParkingWorld::isBlockedByStaticObjects(const glm::vec3& candidate, float yaw) const {
    float halfX = 0.0f, halfZ = 0.0f;
    vehicleHalfExtents(yaw, halfX, halfZ);
    const float columnHalf =
        AeonLayout::COLUMN_SIZE_M * AeonLayout::GEOMETRY_HALF_RATIO + AeonLayout::COLUMN_SAFETY_MARGIN_M;
    for (const auto& column : columns_) {
        const bool overlapX = std::abs(candidate.x - column.x) < halfX + columnHalf;
        const bool overlapZ = std::abs(candidate.z - column.z) < halfZ + columnHalf;
        if (overlapX && overlapZ &&
            std::abs(candidate.y - floorY_) <
                AeonLayout::RAMP_COLLISION_VERTICAL_TOLERANCE_M) {
            return true;
        }
    }
    return false;
}

// Rotate in place with collision checks along the angular sweep. Sampling the
// turn avoids checking only the final 90-degree footprint and missing an
// intermediate overlap.
inline bool ParkingWorld::rotateToward(Vehicle& vehicle, float targetYaw, float dt) {
    float remaining = std::atan2(std::sin(targetYaw - vehicle.yaw_),
                                 std::cos(targetYaw - vehicle.yaw_));
    if (std::abs(remaining) <= OUTBOUND_YAW_TOLERANCE_RAD) {
        vehicle.yaw_ = targetYaw;
        return true;
    }

    const float maxRotation = AUTO_TURN_RATE_RAD_S * dt;
    const float rotationAmount = std::min(std::abs(remaining), maxRotation);
    const float direction = (remaining > 0.0f) ? 1.0f : -1.0f;
    float rotated = 0.0f;

    while (rotated + 1.0e-6f < rotationAmount) {
        const float step = std::min(ROTATION_COLLISION_SUBSTEP_RAD,
                                    rotationAmount - rotated);
        const float candidateYaw = vehicle.yaw_ + direction * step;
        if (isBlockedByTraffic(vehicle.id_, vehicle.position_, candidateYaw) ||
            isBlockedByStaticObjects(vehicle.position_, candidateYaw)) {
            return false;
        }
        vehicle.yaw_ = candidateYaw;
        rotated += step;
    }

    remaining = std::atan2(std::sin(targetYaw - vehicle.yaw_),
                           std::cos(targetYaw - vehicle.yaw_));
    if (std::abs(remaining) <= OUTBOUND_YAW_TOLERANCE_RAD) {
        vehicle.yaw_ = targetYaw;
        return true;
    }
    return false;
}

// Move on the X/Z plane; the Y follow-ramp profile is used while descending.
inline bool ParkingWorld::moveToward(Vehicle& vehicle, const glm::vec3& target, float speed,
                                     float dt, bool reverse, bool followRamp) {
    const glm::vec2 delta(target.x - vehicle.position_.x, target.z - vehicle.position_.z);
    const float distance = glm::length(delta);
    if (distance <= 1.0e-5f) {
        if (followRamp) {
            vehicle.position_.y = AeonLayout::rampYAtX(vehicle.position_.x, upperFloorY_, floorY_);
        }
        return true;
    }

    const glm::vec2 direction = delta / distance;
    const float targetYaw = std::atan2(-direction.y, direction.x);
    // Preserve the established route-following behaviour for ordinary
    // waypoints. The outbound aisle turn is handled explicitly by
    // rotateToward() in AligningOutboundLane, where its swept footprint has
    // enough clearance and can be serialized with other aisle traffic.
    const float candidateYaw = reverse ? vehicle.yaw_ : targetYaw;
    const float step = std::min(speed * dt, distance);
    glm::vec3 candidate = vehicle.position_;
    candidate.x += direction.x * step;
    candidate.z += direction.y * step;
    candidate.y = followRamp ? AeonLayout::rampYAtX(candidate.x, upperFloorY_, floorY_) : floorY_;

    if (isBlockedByTraffic(vehicle.id_, candidate, candidateYaw) ||
        isBlockedByStaticObjects(candidate, candidateYaw)) {
        return false;
    }

    vehicle.position_ = candidate;
    if (!reverse) vehicle.yaw_ = targetYaw;
    return distance <= step + 1.0e-5f;
}

// ------------------------------------------------------------- state machine
inline void ParkingWorld::updateVehicle(Vehicle& vehicle, float dt) {
    const ParkingSlot& slot = slots_[static_cast<std::size_t>(vehicle.slotIndex_)];
    const float floorY = floorY_;
    const glm::vec3 entryGateTarget(AeonLayout::ENTRY_GATE_X_M,
                                    AeonLayout::rampYAtX(AeonLayout::ENTRY_GATE_X_M, upperFloorY_, floorY_),
                                    AeonLayout::ENTRY_LANE_CENTER_Z_M);
    const float entryConnectorTrackX =
        AeonLayout::CONNECTOR_X_M - EXIT_LANE_CLEARANCE_X_M;
    const glm::vec3 connectorEntryTarget(entryConnectorTrackX, floorY,
                                         AeonLayout::ENTRY_LANE_CENTER_Z_M);
    const float entryConnectorHoldX =
        entryConnectorTrackX - EXIT_LANE_CLEARANCE_X_M;
    const glm::vec3 entryConnectorHoldTarget(
        entryConnectorHoldX,
        AeonLayout::rampYAtX(entryConnectorHoldX, upperFloorY_, floorY_),
        AeonLayout::ENTRY_LANE_CENTER_Z_M);
    const glm::vec3 inboundTrackTarget(entryConnectorTrackX, floorY,
                                       AeonLayout::inboundTrackZ(slot.aisleZ()));
    const glm::vec3 bayApproachTarget(slot.center().x, floorY,
                                      AeonLayout::inboundTrackZ(slot.aisleZ()));
    const glm::vec3 bayStagingTarget(
        slot.center().x, floorY,
        slot.aisleZ() + static_cast<float>(slot.rowSide()) *
                            AeonLayout::BAY_TURN_STAGING_OFFSET_M);
    const glm::vec3 aisleCenterTarget(slot.center().x, floorY, slot.aisleZ());
    const glm::vec3 bayTarget(slot.center().x, floorY, slot.center().z);
    const glm::vec3 outboundAlignTarget(slot.center().x, floorY,
                                        AeonLayout::outboundTrackZ(slot.aisleZ()));
    const glm::vec3 exitConnectorTarget(AeonLayout::CONNECTOR_X_M, floorY,
                                        AeonLayout::outboundTrackZ(slot.aisleZ()));
    const float exitLaneClearanceX =
        AeonLayout::CONNECTOR_X_M - EXIT_LANE_CLEARANCE_X_M;
    const glm::vec3 exitLaneClearanceTarget(exitLaneClearanceX, floorY,
                                            AeonLayout::outboundTrackZ(slot.aisleZ()));
    const glm::vec3 exitLaneTarget(exitLaneClearanceX, floorY,
                                   AeonLayout::EXIT_LANE_CENTER_Z_M);
    const glm::vec3 exitGateTarget(AeonLayout::ENTRY_GATE_X_M,
                                   AeonLayout::rampYAtX(AeonLayout::ENTRY_GATE_X_M, upperFloorY_, floorY_),
                                   AeonLayout::EXIT_LANE_CENTER_Z_M);
    const glm::vec3 rampExitTarget(
        AeonLayout::RAMP_START_X_M - AeonLayout::ROUTE_ARRIVAL_EPSILON_M * 2.0f,
        upperFloorY_, AeonLayout::EXIT_LANE_CENTER_Z_M);

    switch (vehicle.phase_) {
    case Vehicle::Phase::EnteringRamp: {
        const glm::vec3 previousPosition = vehicle.position_;
        if (moveToward(vehicle, entryGateTarget,
                       AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt, false, true)) {
            vehicle.phase_ = Vehicle::Phase::EntryGateWait;
            vehicle.phaseTimer_ = AeonLayout::ENTRY_GATE_PREOPEN_SECONDS;
            entryBarrierTimer_ = AeonLayout::BARRIER_OPEN_DURATION_SECONDS;
        } else if (glm::length(glm::vec2(vehicle.position_.x - previousPosition.x,
                                        vehicle.position_.z - previousPosition.z)) <= 1.0e-5f) {
            vehicle.rampQueueWait_ = true;
            vehicle.phase_ = Vehicle::Phase::EntryGateWait;
        }
        break;
    }

    case Vehicle::Phase::EntryGateWait:
        if (vehicle.rampQueueWait_) {
            // A car held by the ramp queue retains the explicit waiting phase
            // while it advances in collision-free increments to the gate.
            if (moveToward(vehicle, entryGateTarget,
                           AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt, false, true)) {
                vehicle.rampQueueWait_ = false;
                vehicle.phaseTimer_ = AeonLayout::ENTRY_GATE_PREOPEN_SECONDS;
                entryBarrierTimer_ = AeonLayout::BARRIER_OPEN_DURATION_SECONDS;
            }
        } else if (vehicle.connectorQueueWait_) {
            if (vehicle.position_.x < entryConnectorHoldX - 1.0e-4f) {
                // Ramp queue: advance toward the protected holding line only
                // when the next footprint-sized step is clear.
                moveToward(vehicle, entryConnectorHoldTarget,
                           AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt, false, true);
            } else if (!hasActiveEntryConnector(vehicle) &&
                       !hasActiveDepartureAtConnector(vehicle) &&
                       !hasActiveDepartureOnAisle(vehicle)) {
                vehicle.connectorQueueWait_ = false;
                vehicle.phase_ = Vehicle::Phase::ToEntryConnector;
            }
        } else {
            vehicle.phaseTimer_ -= dt;
            if (vehicle.phaseTimer_ <= 0.0f) {
                vehicle.phase_ = Vehicle::Phase::ToEntryConnector;
            }
        }
        break;

    case Vehicle::Phase::ToEntryConnector:
        // Stop before the shared entry/exit crossing. If an exit vehicle owns
        // that conflict envelope, do not let a new inbound car queue inside it.
        if (!vehicle.entryConnectorStarted_ &&
            vehicle.position_.x < entryConnectorHoldX - 1.0e-4f) {
            if (!moveToward(vehicle, entryConnectorHoldTarget,
                            AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt, false, true)) {
                vehicle.connectorQueueWait_ = true;
                vehicle.phase_ = Vehicle::Phase::EntryGateWait;
            }
            break;
        }
        if (!vehicle.entryConnectorStarted_) {
            if (hasActiveEntryConnector(vehicle) ||
                hasActiveDepartureAtConnector(vehicle) ||
                hasActiveDepartureOnAisle(vehicle)) {
                // Use the existing explicit waiting phase while queued at the
                // connector hold line, so this is not misreported as a stuck
                // moving vehicle by the simulation watchdog.
                vehicle.connectorQueueWait_ = true;
                vehicle.phase_ = Vehicle::Phase::EntryGateWait;
                break;
            }
            vehicle.entryConnectorStarted_ = true;
        }
        if (moveToward(vehicle, connectorEntryTarget,
                       AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt, false, true)) {
            vehicle.phase_ = Vehicle::Phase::ToTargetAisle;
        }
        break;

    case Vehicle::Phase::ToTargetAisle:
        // Keep the connector reservation until the car has crossed the
        // outbound track at the connector and reached its own inbound track.
        if (moveToward(vehicle, inboundTrackTarget,
                       AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt, false, true)) {
            vehicle.phase_ = Vehicle::Phase::ToBayApproach;
            vehicle.entryConnectorStarted_ = false;
        }
        break;

    case Vehicle::Phase::ToBayApproach:
        if (moveToward(vehicle, bayApproachTarget,
                       AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt, false, true)) {
            vehicle.phase_ = Vehicle::Phase::ToBayStaging;
        }
        break;

    case Vehicle::Phase::ToBayStaging:
        // Small lateral staging movement keeps the car aligned with the aisle.
        // Only after clearing the opposite row's parked-car envelope do we turn
        // 90 degrees and reverse into the selected bay.
        // An admitted outbound car owns the aisle until it reaches the west
        // connector. Conversely, an inbound car already in this phase blocks
        // departure via hasInboundManeuverOnAisle(). This one-way handoff
        // avoids stopping either manoeuvre halfway through the aisle.
        if (hasActiveDepartureOnAisle(vehicle)) break;
        if (moveToward(vehicle, bayStagingTarget, AeonLayout::BAY_MANEUVER_SPEED_MPS,
                       dt, true)) {
            vehicle.phase_ = Vehicle::Phase::ReversingIntoBay;
            vehicle.yaw_ = slot.facingAisleYaw();
        }
        break;

    case Vehicle::Phase::ReversingIntoBay:
        if (moveToward(vehicle, bayTarget, AeonLayout::BAY_MANEUVER_SPEED_MPS, dt, true)) {
            slots_[static_cast<std::size_t>(vehicle.slotIndex_)].occupy(vehicle.id_);
            vehicle.phase_ = Vehicle::Phase::Parked;
            vehicle.phaseTimer_ = dwellDistribution_(randomEngine_);
            vehicle.parkedSeconds_ = 0.0f;
            vehicle.entryConnectorStarted_ = false;
        }
        break;

    case Vehicle::Phase::Parked:
        vehicle.parkedSeconds_ += dt;
        vehicle.phaseTimer_ -= dt;
        if (vehicle.phaseTimer_ <= 0.0f &&
            !hasInboundManeuverOnAisle(vehicle) &&
            !hasActiveDepartureOnAisle(vehicle)) {
            // Remain in the permitted parked/waiting phase until the aisle is
            // actually available; do not start the 30 s movement watchdog while
            // this car is merely queued behind an earlier departure.
            vehicle.departureManeuverStarted_ = true;
            vehicle.phase_ = Vehicle::Phase::LeavingBay;
        }
        break;

    case Vehicle::Phase::LeavingBay:
        // Inbound vehicles have priority to complete their approach / reversing
        // maneuver. This prevents a parked car from backing into an occupied
        // inbound track and creating a symmetric deadlock at the bay mouth.
        // Outbound maneuvers are serialized by vehicle id within each aisle.
        // Acquire right-of-way once. Rechecking the inbound phase every frame
        // can stop a car halfway through the aisle after a new arrival appears.
        if (!vehicle.departureManeuverStarted_) {
            if (hasInboundManeuverOnAisle(vehicle) ||
                hasActiveDepartureOnAisle(vehicle)) {
                break;
            }
            vehicle.departureManeuverStarted_ = true;
        }
        if (moveToward(vehicle, aisleCenterTarget, AeonLayout::EXIT_MANEUVER_SPEED_MPS, dt)) {
            vehicle.phase_ = Vehicle::Phase::AligningOutboundLane;
        }
        break;

    case Vehicle::Phase::AligningOutboundLane:
        // First turn the car parallel to the outbound aisle (-X) while it is
        // centred in the 6 m aisle. Only then make the short lane offset; this
        // keeps the long vehicle footprint out of the opposing parked row.
        if (!rotateToward(vehicle, AeonLayout::PI, dt)) break;
        // This is a constrained lateral lane-centering step in the simplified
        // waypoint model. Preserve the outbound heading during the offset.
        if (moveToward(vehicle, outboundAlignTarget,
                       AeonLayout::EXIT_MANEUVER_SPEED_MPS, dt, true)) {
            vehicle.phase_ = Vehicle::Phase::ToExitConnector;
        }
        break;

    case Vehicle::Phase::ToExitConnector:
        if (hasActiveEntryConnector(vehicle)) break;
        if (moveToward(vehicle, exitConnectorTarget,
                       AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt)) {
            vehicle.phase_ = Vehicle::Phase::ToExitLane;
        }
        break;

    case Vehicle::Phase::ToExitLane:
        // First gain a complete vehicle-length of longitudinal clearance on
        // the outbound track. Moving laterally into the paired ramp lane at
        // CONNECTOR_X made this car overlap the following car on the aisle.
        if (vehicle.position_.x > exitLaneClearanceX + 1.0e-4f) {
            if (moveToward(vehicle, exitLaneClearanceTarget,
                           AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt, false, true)) {
                // The car has cleared the aisle itself. Keep its departure
                // reservation until it also passes the shared ramp merge.
                releaseSlot(vehicle);
            }
        } else {
            // The two aisles converge into one physical exit lane. Admit one
            // lateral merge at a time; release the merge right after the car
            // has advanced one clear vehicle-length along the common lane.
            if (!vehicle.exitMergeStarted_) {
                if (hasActiveExitMerge(vehicle) ||
                    hasActiveEntryConnector(vehicle)) break;
                vehicle.exitMergeStarted_ = true;
            }
            if (moveToward(vehicle, exitLaneTarget,
                           AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt, true, true)) {
                vehicle.phase_ = Vehicle::Phase::ExitGateApproach;
            }
        }
        break;

    case Vehicle::Phase::ExitGateApproach:
        if (moveToward(vehicle, exitGateTarget, AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt,
                       false, true)) {
            vehicle.phase_ = Vehicle::Phase::ExitPaymentWait;
            vehicle.phaseTimer_ = AeonLayout::EXIT_PAYMENT_WAIT_SECONDS;
            vehicle.exitMergeStarted_ = false;
            vehicle.departureManeuverStarted_ = false;
        } else if (vehicle.exitMergeStarted_ &&
                   vehicle.position_.x <= exitLaneClearanceX - EXIT_LANE_CLEARANCE_X_M) {
            vehicle.exitMergeStarted_ = false;
            vehicle.departureManeuverStarted_ = false;
        }
        break;

    case Vehicle::Phase::ExitPaymentWait:
        vehicle.phaseTimer_ -= dt;
        if (vehicle.phaseTimer_ <= 0.0f) {
            exitBarrierTimer_ = AeonLayout::BARRIER_OPEN_DURATION_SECONDS;
            vehicle.phase_ = Vehicle::Phase::ExitingRamp;
        }
        break;

    case Vehicle::Phase::ExitingRamp:
        if (moveToward(vehicle, rampExitTarget, AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt,
                       false, true)) {
            vehicle.active_ = false;
        }
        break;
    }
}

inline void ParkingWorld::update(float dt) {
    if (dt <= 0.0f) return;
    updateMotorbikes(dt);
    entryBarrierTimer_ = std::max(0.0f, entryBarrierTimer_ - dt);
    exitBarrierTimer_ = std::max(0.0f, exitBarrierTimer_ - dt);

    for (auto& vehicle : traffic_) {
        if (!vehicle.active_ || vehicle.slotIndex_ < 0 ||
            vehicle.slotIndex_ >= static_cast<int>(slots_.size())) {
            continue;
        }
        updateVehicle(vehicle, dt);
    }

    traffic_.erase(std::remove_if(traffic_.begin(), traffic_.end(),
        [](const Vehicle& vehicle) { return !vehicle.active_; }), traffic_.end());
}

inline void ParkingWorld::updateManualCar(const ManualControl& control, float dt) {
    if (!manualMode_) return;

    playerYaw_ += control.steer * AeonLayout::MANUAL_TURN_SPEED_RAD_S * dt;
    if (control.throttle == 0.0f) return;

    const glm::vec3 direction(std::cos(playerYaw_), 0.0f, -std::sin(playerYaw_));
    glm::vec3 candidate =
        playerPosition_ + direction * (control.throttle * AeonLayout::MANUAL_DRIVE_SPEED_MPS * dt);
    candidate.x = glm::clamp(candidate.x,
                             AeonLayout::ZONE_MIN_X_M + AeonLayout::PLAYER_BOUNDARY_MARGIN_M,
                             AeonLayout::ZONE_MAX_X_M - AeonLayout::PLAYER_BOUNDARY_MARGIN_M);
    candidate.z = glm::clamp(candidate.z,
                             AeonLayout::ZONE_MIN_Z_M + AeonLayout::PLAYER_BOUNDARY_MARGIN_M,
                             AeonLayout::ZONE_MAX_Z_M - AeonLayout::PLAYER_BOUNDARY_MARGIN_M);

    bool blocked = isBlockedByStaticObjects(candidate, playerYaw_);
    for (const auto& vehicle : traffic_) {
        if (overlapsFootprints(candidate, playerYaw_, vehicle.position(), vehicle.yaw())) {
            blocked = true;
            break;
        }
    }
    if (!blocked) playerPosition_ = candidate;
}
