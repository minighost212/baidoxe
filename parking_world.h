#pragma once

// ============================================================================
//  parking_world.h - simulation model for a single basement level.
//  Each ParkingWorld instance owns its own cars, motorbike demonstration bays,
//  barriers and floor geometry. B1 and B2 are kept as separate worlds.
// ============================================================================

#include <algorithm>
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

    int id_ = -1;
    glm::vec3 position_{0.0f};
    float yaw_ = 0.0f;
    int slotIndex_ = -1;
    Phase phase_ = Phase::EnteringRamp;
    float phaseTimer_ = 0.0f;
    float parkedSeconds_ = 0.0f;
    bool active_ = true;
};

struct ParkedMotorbike {
    glm::vec3 position{0.0f};
    float yaw = 0.0f;
    int slotIndex = -1;
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
    bool spawnMotorbike();
    bool removeLastMotorbike();
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
    void updateVehicle(Vehicle& vehicle, float dt);
    void releaseSlot(const Vehicle& vehicle);
    bool moveToward(Vehicle& vehicle, const glm::vec3& target, float speed, float dt,
                    bool reverse = false, bool followRamp = false);
    bool isBlockedByTraffic(int ignoreVehicleId, const glm::vec3& candidate, float yaw) const;
    bool isBlockedByStaticObjects(const glm::vec3& candidate, float yaw) const;
    bool hasInboundManeuverOnAisle(const Vehicle& departing) const;
    bool hasEarlierDepartureOnAisle(const Vehicle& departing) const;
    bool overlapsFootprints(const glm::vec3& aPosition, float aYaw,
                            const glm::vec3& bPosition, float bYaw) const;
    static void vehicleHalfExtents(float yaw, float& halfX, float& halfZ);
    int findFreeSlot() const;

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

    bool manualMode_ = false;
    glm::vec3 playerPosition_{0.0f};
    float playerYaw_ = AeonLayout::PI;

    std::mt19937 randomEngine_;
    std::uniform_real_distribution<float> dwellDistribution_{
        AeonLayout::PARKING_DWELL_MIN_SECONDS, AeonLayout::PARKING_DWELL_MAX_SECONDS};
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
    entryBarrierTimer_ = exitBarrierTimer_ = spawnTimer_ = 0.0f;
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

inline bool ParkingWorld::hasTrafficCollisions() const {
    for (std::size_t i = 0; i < traffic_.size(); ++i) {
        if (!traffic_[i].isActive()) continue;
        for (std::size_t j = i + 1; j < traffic_.size(); ++j) {
            if (!traffic_[j].isActive()) continue;
            if (overlapsFootprints(traffic_[i].position(), traffic_[i].yaw(),
                                   traffic_[j].position(), traffic_[j].yaw())) return true;
        }
        if (manualMode_ && overlapsFootprints(traffic_[i].position(), traffic_[i].yaw(),
                                              playerPosition_, playerYaw_)) return true;
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
        const float yaw = (row == 0) ? 0.0f : AeonLayout::PI;
        motorbikes_.push_back({motorbikeSlotCenters_[index], yaw, static_cast<int>(index)});
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
    if (slot.vehicleId() == vehicle.id_) slot.reset();
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
    for (const auto& other : traffic_) {
        if (other.id_ == ignoreVehicleId || !other.active_) continue;
        if (overlapsFootprints(candidate, yaw, other.position_, other.yaw_)) return true;
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
        if (other.phase_ == Vehicle::Phase::ToBayApproach ||
            other.phase_ == Vehicle::Phase::ToBayStaging ||
            other.phase_ == Vehicle::Phase::ReversingIntoBay) return true;
    }
    return false;
}

inline bool ParkingWorld::hasEarlierDepartureOnAisle(const Vehicle& departing) const {
    if (departing.slotIndex_ < 0 ||
        departing.slotIndex_ >= static_cast<int>(slots_.size())) return false;
    const float aisleZ = slots_[static_cast<std::size_t>(departing.slotIndex_)].aisleZ();
    for (const auto& other : traffic_) {
        if (!other.active_ || other.id_ >= departing.id_ || other.slotIndex_ < 0 ||
            other.slotIndex_ >= static_cast<int>(slots_.size())) continue;
        if (std::abs(slots_[static_cast<std::size_t>(other.slotIndex_)].aisleZ() - aisleZ) >
            AeonLayout::ROUTE_ARRIVAL_EPSILON_M) continue;
        if (other.phase_ == Vehicle::Phase::LeavingBay ||
            other.phase_ == Vehicle::Phase::AligningOutboundLane) return true;
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

// Move on the X/Z plane; the Y follow-ramp profile is used while descending.
inline bool ParkingWorld::moveToward(Vehicle& vehicle, const glm::vec3& target, float speed,
                                     float dt, bool reverse, bool followRamp) {
    const glm::vec2 delta(target.x - vehicle.position_.x, target.z - vehicle.position_.z);
    const float distance = glm::length(delta);
    if (distance < AeonLayout::ROUTE_ARRIVAL_EPSILON_M) {
        vehicle.position_ = target;
        if (followRamp) vehicle.position_.y = AeonLayout::rampYAtX(vehicle.position_.x, upperFloorY_, floorY_);
        return true;
    }

    const glm::vec2 direction = delta / distance;
    const float targetYaw = std::atan2(-direction.y, direction.x);
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
    return distance <= step + AeonLayout::ROUTE_ARRIVAL_EPSILON_M;
}

// ------------------------------------------------------------- state machine
inline void ParkingWorld::updateVehicle(Vehicle& vehicle, float dt) {
    const ParkingSlot& slot = slots_[static_cast<std::size_t>(vehicle.slotIndex_)];
    const float floorY = floorY_;

    // -----------------------------------------------------------------------
    // Luồng xe thực tế tầng hầm:
    //  Xe vào từ dốc (W) → đi thẳng ĐÔNG dọc inbound lane (Z=-1.75m) →
    //  đến vị trí X của lô → quẹo 90° vào lô (head-in, tiến vào đầu xe).
    //  Khi ra: lùi ra làn outbound (Z=+1.75m) → đi Tây → ra cổng → lên dốc.
    // -----------------------------------------------------------------------

    const glm::vec3 entryGateTarget(AeonLayout::ENTRY_GATE_X_M,
                                    AeonLayout::rampYAtX(AeonLayout::ENTRY_GATE_X_M, upperFloorY_, floorY_),
                                    AeonLayout::ENTRY_LANE_CENTER_Z_M);

    // Điểm xe vào bên trong hầm (sau barem) - vẫn trên inbound lane
    const glm::vec3 connectorEntryTarget(AeonLayout::CONNECTOR_X_M, floorY,
                                         AeonLayout::ENTRY_LANE_CENTER_Z_M);

    // Xe đi thẳng Đông dọc inbound lane đến cột X của lô mình
    const glm::vec3 bayApproachTarget(slot.center().x, floorY,
                                      AeonLayout::ENTRY_LANE_CENTER_Z_M);

    // Xe rẽ 90° sang phía lô (N hoặc S) đến rìa aisle tiếp giáp lô
    const glm::vec3 bayStagingTarget(
        slot.center().x, floorY,
        slot.aisleZ() - static_cast<float>(slot.rowSide()) * AeonLayout::BAY_TURN_STAGING_OFFSET_M);

    // Điểm đỗ trong lô (đầu xe vào trong)
    const glm::vec3 bayTarget(slot.center().x, floorY, slot.center().z);

    // Lùi ra giữa aisle khi rời lô
    const glm::vec3 aisleCenterTarget(slot.center().x, floorY, slot.aisleZ());

    // Lùi ra đến outbound lane
    const glm::vec3 outboundAlignTarget(slot.center().x, floorY,
                                        AeonLayout::EXIT_LANE_CENTER_Z_M);

    // Xe đi Tây dọc outbound lane đến connector
    const glm::vec3 exitConnectorTarget(AeonLayout::CONNECTOR_X_M, floorY,
                                        AeonLayout::EXIT_LANE_CENTER_Z_M);

    // Tiếp tục Tây đến exit gate
    const glm::vec3 exitLaneTarget(AeonLayout::CONNECTOR_X_M, floorY,
                                   AeonLayout::EXIT_LANE_CENTER_Z_M);
    const glm::vec3 exitGateTarget(AeonLayout::ENTRY_GATE_X_M,
                                   AeonLayout::rampYAtX(AeonLayout::ENTRY_GATE_X_M, upperFloorY_, floorY_),
                                   AeonLayout::EXIT_LANE_CENTER_Z_M);
    const glm::vec3 rampExitTarget(
        AeonLayout::RAMP_START_X_M - AeonLayout::ROUTE_ARRIVAL_EPSILON_M * 2.0f,
        upperFloorY_, AeonLayout::EXIT_LANE_CENTER_Z_M);

    switch (vehicle.phase_) {
    case Vehicle::Phase::EnteringRamp:
        if (moveToward(vehicle, entryGateTarget, AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt,
                       false, true)) {
            vehicle.phase_ = Vehicle::Phase::EntryGateWait;
            vehicle.phaseTimer_ = AeonLayout::ENTRY_GATE_PREOPEN_SECONDS;
            entryBarrierTimer_ = AeonLayout::BARRIER_OPEN_DURATION_SECONDS;
        }
        break;

    case Vehicle::Phase::EntryGateWait:
        vehicle.phaseTimer_ -= dt;
        if (vehicle.phaseTimer_ <= 0.0f) vehicle.phase_ = Vehicle::Phase::ToEntryConnector;
        break;

    case Vehicle::Phase::ToEntryConnector:
        // Qua barem, tiếp tục thẳng Đông trên inbound lane
        if (moveToward(vehicle, connectorEntryTarget, AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt,
                       false, true)) {
            vehicle.phase_ = Vehicle::Phase::ToTargetAisle;
        }
        break;

    case Vehicle::Phase::ToTargetAisle:
        // Vẫn đi thẳng Đông dọc inbound lane đến X của lô mình
        if (moveToward(vehicle, bayApproachTarget, AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt)) {
            vehicle.phase_ = Vehicle::Phase::ToBayApproach;
        }
        break;

    case Vehicle::Phase::ToBayApproach:
        // Rẽ 90° vào aisle, tiến về phía lô (staging trước khi vào hẳn)
        if (moveToward(vehicle, bayStagingTarget, AeonLayout::BAY_MANEUVER_SPEED_MPS, dt)) {
            vehicle.phase_ = Vehicle::Phase::ToBayStaging;
        }
        break;

    case Vehicle::Phase::ToBayStaging:
        // Tiến thẳng đầu xe vào lô (head-in parking)
        {
            // Yaw hướng xe vào lô: nếu rowSide=+1 (Bắc, Z lớn) thì xe nhìn về +Z (PI/2)
            //                       nếu rowSide=-1 (Nam, Z nhỏ) thì xe nhìn về -Z (-PI/2)
            const float headInYaw = (slot.rowSide() > 0) ? AeonLayout::PI * 0.5f
                                                          : -AeonLayout::PI * 0.5f;
            vehicle.yaw_ = headInYaw;
        }
        if (moveToward(vehicle, bayTarget, AeonLayout::BAY_MANEUVER_SPEED_MPS, dt)) {
            vehicle.phase_ = Vehicle::Phase::ReversingIntoBay;
        }
        break;

    case Vehicle::Phase::ReversingIntoBay:
        // Xe đã vào lô xong – chiếm lô
        {
            slots_[static_cast<std::size_t>(vehicle.slotIndex_)].occupy(vehicle.id_);
            vehicle.phase_ = Vehicle::Phase::Parked;
            vehicle.phaseTimer_ = dwellDistribution_(randomEngine_);
            vehicle.parkedSeconds_ = 0.0f;
        }
        break;

    case Vehicle::Phase::Parked:
        vehicle.parkedSeconds_ += dt;
        vehicle.phaseTimer_ -= dt;
        if (vehicle.phaseTimer_ <= 0.0f) vehicle.phase_ = Vehicle::Phase::LeavingBay;
        break;

    case Vehicle::Phase::LeavingBay:
        // Inbound vehicles have priority to complete their approach / reversing
        // maneuver. This prevents a parked car from backing into an occupied
        // inbound track and creating a symmetric deadlock at the bay mouth.
        // Outbound maneuvers are serialized by vehicle id within each aisle.
        if (!hasInboundManeuverOnAisle(vehicle) &&
            !hasEarlierDepartureOnAisle(vehicle) &&
            moveToward(vehicle, aisleCenterTarget, AeonLayout::EXIT_MANEUVER_SPEED_MPS, dt)) {
            vehicle.phase_ = Vehicle::Phase::AligningOutboundLane;
        }
        break;

    case Vehicle::Phase::AligningOutboundLane:
        if (moveToward(vehicle, outboundAlignTarget, AeonLayout::EXIT_MANEUVER_SPEED_MPS, dt)) {
            vehicle.phase_ = Vehicle::Phase::ToExitConnector;
            releaseSlot(vehicle);
        }
        break;

    case Vehicle::Phase::ToExitConnector:
        if (moveToward(vehicle, exitConnectorTarget, AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt)) {
            vehicle.phase_ = Vehicle::Phase::ToExitLane;
        }
        break;

    case Vehicle::Phase::ToExitLane:
        if (moveToward(vehicle, exitLaneTarget, AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt,
                       false, true)) {
            vehicle.phase_ = Vehicle::Phase::ExitGateApproach;
        }
        break;

    case Vehicle::Phase::ExitGateApproach:
        if (moveToward(vehicle, exitGateTarget, AeonLayout::AUTO_VEHICLE_SPEED_MPS, dt,
                       false, true)) {
            vehicle.phase_ = Vehicle::Phase::ExitPaymentWait;
            vehicle.phaseTimer_ = AeonLayout::EXIT_PAYMENT_WAIT_SECONDS;
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
