// ============================================================================
//  camera.cpp - Implementation of the Object-Oriented Camera System.
// ============================================================================

#include "camera.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float PI = 3.14159265358979323846f;

inline float normalizeAngle(float angle) {
    while (angle > PI) angle -= 2.0f * PI;
    while (angle < -PI) angle += 2.0f * PI;
    return angle;
}

inline float lerpAngle(float from, float to, float alpha) {
    float diff = normalizeAngle(to - from);
    return from + diff * alpha;
}
} // namespace

// ============================================================================
//  OrbitView Implementation
// ============================================================================
OrbitView::OrbitView(float yaw, float pitch, float distance,
                     const glm::vec3& target, const char* name, const char* hint)
    : yaw_(yaw), pitch_(pitch), distance_(distance),
      target_(target), name_(name), hint_(hint) {}

void OrbitView::update(const CameraContext& ctx) {
    (void)ctx;
}

glm::vec3 OrbitView::getPosition() const {
    return target_ + distance_ * glm::vec3(
        std::cos(pitch_) * std::cos(yaw_),
        std::sin(pitch_),
        std::cos(pitch_) * std::sin(yaw_));
}

void OrbitView::onMouseOrbit(float deltaX, float deltaY) {
    yaw_ += deltaX * AeonLayout::CAMERA_ROTATION_DELTA_SCALE;
    pitch_ = std::clamp(pitch_ + deltaY * AeonLayout::CAMERA_ROTATION_DELTA_SCALE,
                        AeonLayout::CAMERA_MIN_PITCH_RAD,
                        AeonLayout::CAMERA_MAX_PITCH_RAD);
}

void OrbitView::onZoom(double wheelDelta) {
    const float factor = wheelDelta > 0.0
                             ? AeonLayout::CAMERA_ZOOM_IN_FACTOR
                             : AeonLayout::CAMERA_ZOOM_OUT_FACTOR;
    distance_ = std::clamp(distance_ * factor,
                           AeonLayout::CAMERA_MIN_DISTANCE_M,
                           AeonLayout::CAMERA_MAX_DISTANCE_M);
}

// ============================================================================
//  FirstPersonWalkView Implementation
// ============================================================================
FirstPersonWalkView::FirstPersonWalkView() {
    position_ = glm::vec3(AeonLayout::RAMP_END_X_M + 2.0f,
                          currentFloorY_ + eyeHeight_,
                          AeonLayout::ENTRY_LANE_CENTER_Z_M);
}

void FirstPersonWalkView::setFloorY(float floorY) {
    currentFloorY_ = floorY;
    position_.y = currentFloorY_ + eyeHeight_ + bobOffset_;
    clampWithinBasement();
}

void FirstPersonWalkView::enter(const CameraContext& ctx) {
    currentFloorY_ = ctx.floorY;
    // Đặt vị trí ban đầu gần lối xe vào hầm nếu chưa ở trong hầm
    if (position_.x < AeonLayout::ZONE_MIN_X_M - 10.0f ||
        position_.x > AeonLayout::ZONE_MAX_X_M + 5.0f) {
        position_ = glm::vec3(AeonLayout::RAMP_END_X_M + 3.0f,
                              currentFloorY_ + eyeHeight_,
                              AeonLayout::ENTRY_LANE_CENTER_Z_M);
        yaw_ = 0.0f; // Nhìn về phía trong hầm
        pitch_ = -0.05f;
    } else {
        position_.y = currentFloorY_ + eyeHeight_;
    }
}

void FirstPersonWalkView::update(const CameraContext& ctx) {
    currentFloorY_ = ctx.floorY;

    const float moveSpeed = ctx.isSprinting ? 6.5f : 3.5f;

    // Vector hướng nhìn trên mặt phẳng ngang XZ
    const glm::vec3 forward(std::cos(yaw_), 0.0f, std::sin(yaw_));
    const glm::vec3 right(-std::sin(yaw_), 0.0f, std::cos(yaw_));

    glm::vec3 moveDir(0.0f);
    if (ctx.moveForward != 0.0f) {
        moveDir += forward * ctx.moveForward;
    }
    if (ctx.moveRight != 0.0f) {
        moveDir += right * ctx.moveRight;
    }

    if (glm::length(moveDir) > 0.001f) {
        moveDir = glm::normalize(moveDir);
        position_ += moveDir * (moveSpeed * ctx.dt);
        isMoving_ = true;
        bobTimer_ += ctx.dt * (ctx.isSprinting ? 12.0f : 8.5f);
        bobOffset_ = std::sin(bobTimer_) * 0.035f;
    } else {
        isMoving_ = false;
        bobOffset_ *= 0.85f; // Giảm dần nhịp bước chân khi dừng
    }

    position_.y = currentFloorY_ + eyeHeight_ + bobOffset_;
    clampWithinBasement();
}

void FirstPersonWalkView::clampWithinBasement() {
    // Giới hạn trong khuôn viên tầng hầm (không rơi ra ngoài và không bay lên trần)
    const float minX = AeonLayout::RAMP_START_X_M - 2.0f;
    const float maxX = AeonLayout::ZONE_MAX_X_M + 1.0f;
    const float minZ = AeonLayout::ZONE_MIN_Z_M - 1.0f;
    const float maxZ = AeonLayout::BIKE_ZONE_CENTER_Z_M +
                       AeonLayout::BIKE_ZONE_DEPTH_Z_M * 0.5f + 1.0f;

    position_.x = std::clamp(position_.x, minX, maxX);
    position_.z = std::clamp(position_.z, minZ, maxZ);

    const float ceilingY = currentFloorY_ + AeonLayout::FLOOR_TO_CEILING_M - 0.2f;
    const float floorClampY = currentFloorY_ + 0.5f;
    position_.y = std::clamp(position_.y, floorClampY, ceilingY);
}

glm::vec3 FirstPersonWalkView::getPosition() const {
    return position_;
}

glm::vec3 FirstPersonWalkView::getTarget() const {
    const glm::vec3 lookDir(
        std::cos(pitch_) * std::cos(yaw_),
        std::sin(pitch_),
        std::cos(pitch_) * std::sin(yaw_));
    return position_ + lookDir;
}

void FirstPersonWalkView::onMouseOrbit(float deltaX, float deltaY) {
    yaw_ += deltaX * AeonLayout::CAMERA_ROTATION_DELTA_SCALE;
    pitch_ = std::clamp(pitch_ + deltaY * AeonLayout::CAMERA_ROTATION_DELTA_SCALE,
                        -1.45f, 1.45f); // ~ +/- 83 độ
}

// ============================================================================
//  VehicleCockpitView Implementation
// ============================================================================
VehicleCockpitView::VehicleCockpitView() {
    currentPos_ = glm::vec3(0.0f, AeonLayout::FLOOR_HEIGHT_B1_M + 1.15f, 0.0f);
}

void VehicleCockpitView::update(const CameraContext& ctx) {
    if (ctx.hasVehicle) {
        smoothYaw_ = lerpAngle(smoothYaw_, ctx.vehicleYaw, std::min(1.0f, ctx.dt * 15.0f));

        // Vị trí ghế lái tài xế trong xe Vios: lùi 0.35m, lệch trái 0.35m, cao 1.15m
        const glm::vec3 fwd(std::cos(smoothYaw_), 0.0f, std::sin(smoothYaw_));
        const glm::vec3 left(-std::sin(smoothYaw_), 0.0f, std::cos(smoothYaw_));

        currentPos_ = ctx.vehiclePosition - fwd * 0.35f - left * 0.35f + glm::vec3(0.0f, 1.15f, 0.0f);
        // Nhìn thẳng về phía trước qua kính chắn gió
        currentTarget_ = currentPos_ + fwd * 10.0f + glm::vec3(0.0f, -0.1f, 0.0f);
    } else {
        currentPos_ = glm::vec3(0.0f, ctx.floorY + 1.15f, 0.0f);
        currentTarget_ = currentPos_ + glm::vec3(5.0f, 0.0f, 0.0f);
    }
}

glm::vec3 VehicleCockpitView::getPosition() const {
    return currentPos_;
}

glm::vec3 VehicleCockpitView::getTarget() const {
    return currentTarget_;
}

// ============================================================================
//  VehicleChaseView Implementation
// ============================================================================
VehicleChaseView::VehicleChaseView() {
    currentPos_ = glm::vec3(-5.0f, AeonLayout::FLOOR_HEIGHT_B1_M + 2.0f, 0.0f);
}

void VehicleChaseView::update(const CameraContext& ctx) {
    if (ctx.hasVehicle) {
        smoothYaw_ = lerpAngle(smoothYaw_, ctx.vehicleYaw, std::min(1.0f, ctx.dt * 8.0f));

        const glm::vec3 fwd(std::cos(smoothYaw_), 0.0f, std::sin(smoothYaw_));
        // Đặt phía sau đuôi xe 4.5m, cao 1.85m
        currentPos_ = ctx.vehiclePosition - fwd * 4.5f + glm::vec3(0.0f, 1.85f, 0.0f);
        // Điểm ngắm: Mũi xe phía trước
        currentTarget_ = ctx.vehiclePosition + fwd * 2.0f + glm::vec3(0.0f, 0.8f, 0.0f);
    } else {
        currentPos_ = glm::vec3(-5.0f, ctx.floorY + 2.0f, 0.0f);
        currentTarget_ = glm::vec3(0.0f, ctx.floorY + 1.0f, 0.0f);
    }
}

glm::vec3 VehicleChaseView::getPosition() const {
    return currentPos_;
}

glm::vec3 VehicleChaseView::getTarget() const {
    return currentTarget_;
}

// ============================================================================
//  BasementCctvView Implementation
// ============================================================================
BasementCctvView::BasementCctvView() {
    setupChannels();
}

void BasementCctvView::setupChannels() {
    channels_.clear();

    // Kênh 1: Cổng soát vé & Dốc vào hầm (Góc nhìn toàn cảnh trạm gác chân dốc)
    channels_.push_back({
        "CCTV 01 - TRAM SOAT VE CHAN DOC (RAMP FOOT CHECKPOINT)",
        glm::vec3(AeonLayout::ZONE_MIN_X_M - 4.8f, 2.75f, 4.2f),
        glm::vec3(AeonLayout::ZONE_MIN_X_M + 1.2f, 0.75f, 0.0f),
        0.12f, 0.40f
    });

    // Kênh 2: Cận cảnh bàn bảo vệ & barrier trạm gác
    channels_.push_back({
        "CCTV 02 - CAN CANH BAN BAO VE & BARRIER (GUARD BOOTH)",
        glm::vec3(AeonLayout::ZONE_MIN_X_M + 3.8f, 2.10f, -2.8f),
        glm::vec3(AeonLayout::ZONE_MIN_X_M + 1.2f, 0.90f, 0.0f),
        0.10f, 0.35f
    });

    // Kênh 3: Giao lộ trục đường chính tầng hầm (Aisle Center)
    channels_.push_back({
        "CCTV 03 - GIAO LO TRUC CHINH TANG HAM (AISLE 1)",
        glm::vec3(AeonLayout::CORE_MIN_X_M + 1.0f, 2.85f, AeonLayout::ZONE_MIN_Z_M + 1.2f),
        glm::vec3(AeonLayout::WORLD_ORIGIN_X_M, 0.7f, AeonLayout::AISLE_CENTER_Z_M[0]),
        0.14f, 0.42f
    });

    // Kênh 4: Sảnh thang máy trung tâm (Elevator Lobby)
    channels_.push_back({
        "CCTV 04 - SANH THANG MAY TRUNG TAM (LOBBY)",
        glm::vec3(AeonLayout::ELEVATOR_CORE_CENTER_X_M - 1.5f, 2.85f, AeonLayout::ELEVATOR_CORE_CENTER_Z_M + 3.0f),
        glm::vec3(AeonLayout::ELEVATOR_CORE_CENTER_X_M, 1.1f, AeonLayout::ELEVATOR_CORE_CENTER_Z_M),
        0.08f, 0.30f
    });

    // Kênh 5: Bãi đỗ xe máy phụ trợ
    channels_.push_back({
        "CCTV 05 - KHU VUC DO XE MAY (MOTORBIKE AREA)",
        glm::vec3(AeonLayout::BIKE_ZONE_CENTER_X_M - 5.5f, 2.85f, AeonLayout::BIKE_ZONE_CENTER_Z_M - 3.0f),
        glm::vec3(AeonLayout::BIKE_ZONE_CENTER_X_M, 0.8f, AeonLayout::BIKE_ZONE_CENTER_Z_M),
        0.12f, 0.38f
    });
}

void BasementCctvView::enter(const CameraContext& ctx) {
    currentFloorY_ = ctx.floorY;
}

void BasementCctvView::update(const CameraContext& ctx) {
    currentFloorY_ = ctx.floorY;
    sweepTimer_ += ctx.dt;
}

glm::vec3 BasementCctvView::getPosition() const {
    if (channels_.empty()) return glm::vec3(0.0f);
    const auto& ch = channels_[currentChannel_];
    return glm::vec3(ch.ceilingOffset.x,
                     currentFloorY_ + ch.ceilingOffset.y,
                     ch.ceilingOffset.z);
}

glm::vec3 BasementCctvView::getTarget() const {
    if (channels_.empty()) return glm::vec3(0.0f, 0.0f, 1.0f);
    const auto& ch = channels_[currentChannel_];
    const glm::vec3 pos = getPosition();
    glm::vec3 baseTarget(ch.lookAtTarget.x,
                         currentFloorY_ + ch.lookAtTarget.y,
                         ch.lookAtTarget.z);

    // Hiệu ứng lia góc tự động chậm rãi (security camera sweep pan)
    const float panAngle = std::sin(sweepTimer_ * ch.sweepSpeed) * ch.sweepAmplitudeRad;
    const glm::vec3 toTarget = baseTarget - pos;
    const glm::mat4 rot = glm::rotate(glm::mat4(1.0f), panAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 sweptDir = glm::vec3(rot * glm::vec4(toTarget, 0.0f));

    return pos + sweptDir;
}

const char* BasementCctvView::getName() const {
    if (currentChannel_ >= 0 && currentChannel_ < static_cast<int>(channels_.size())) {
        currentDisplayName_ = channels_[currentChannel_].name;
    } else {
        currentDisplayName_ = "BASEMENT CCTV SURVEILLANCE";
    }
    return currentDisplayName_.c_str();
}

void BasementCctvView::nextChannel() {
    if (!channels_.empty()) {
        currentChannel_ = (currentChannel_ + 1) % static_cast<int>(channels_.size());
    }
}

void BasementCctvView::prevChannel() {
    if (!channels_.empty()) {
        currentChannel_ = (currentChannel_ - 1 + static_cast<int>(channels_.size())) % static_cast<int>(channels_.size());
    }
}

// ============================================================================
//  TopDownView Implementation
// ============================================================================
TopDownView::TopDownView() = default;

void TopDownView::update(const CameraContext& ctx) {
    (void)ctx;
}

glm::vec3 TopDownView::getPosition() const {
    return glm::vec3(target_.x,
                     target_.y + height_,
                     target_.z + AeonLayout::CAMERA_TOP_VIEW_Z_OFFSET_M);
}

glm::vec3 TopDownView::getTarget() const {
    return target_;
}

// ============================================================================
//  Camera Facade Implementation
// ============================================================================
Camera::Camera() {
    exteriorOrbit_ = std::make_unique<OrbitView>(
        AeonLayout::CAMERA_INITIAL_YAW_RAD,
        AeonLayout::CAMERA_INITIAL_PITCH_RAD,
        AeonLayout::CAMERA_INITIAL_DISTANCE_M,
        glm::vec3(AeonLayout::MALL_CAMERA_TARGET_X_M,
                  AeonLayout::MALL_CAMERA_TARGET_Y_M,
                  AeonLayout::WORLD_ORIGIN_Z_M),
        "EXTERIOR MALL ORBIT (NGOAI THAT TTTH)",
        "CHUOT TRAI: XOAY SA BAN | CUON CHUOT: ZOOM");

    basementOrbit_ = std::make_unique<OrbitView>(
        AeonLayout::CAMERA_BASEMENT_INITIAL_YAW_RAD,
        AeonLayout::CAMERA_BASEMENT_INITIAL_PITCH_RAD,
        AeonLayout::CAMERA_BASEMENT_INITIAL_DISTANCE_M,
        glm::vec3(AeonLayout::CAMERA_BASEMENT_TARGET_X_M,
                  AeonLayout::BASEMENT_CAMERA_TARGET_Y_M,
                  AeonLayout::CAMERA_BASEMENT_TARGET_Z_M),
        "BASEMENT ORBIT (SA BAN TANG HAM)",
        "CHUOT TRAI: XOAY SA BAN | CUON CHUOT: ZOOM");

    firstPersonWalk_ = std::make_unique<FirstPersonWalkView>();
    vehicleCockpit_ = std::make_unique<VehicleCockpitView>();
    vehicleChase_ = std::make_unique<VehicleChaseView>();
    basementCctv_ = std::make_unique<BasementCctvView>();
    topDown_ = std::make_unique<TopDownView>();

    activeStrategy_ = exteriorOrbit_.get();
    currentMode_ = CameraMode::ExteriorOrbit;
}

Camera::~Camera() = default;

void Camera::setMode(CameraMode mode, const CameraContext& ctx) {
    currentMode_ = mode;
    switch (mode) {
    case CameraMode::ExteriorOrbit:
        activeStrategy_ = exteriorOrbit_.get();
        break;
    case CameraMode::BasementOrbit:
        activeStrategy_ = basementOrbit_.get();
        previousBasementMode_ = CameraMode::BasementOrbit;
        break;
    case CameraMode::FirstPersonWalk:
        activeStrategy_ = firstPersonWalk_.get();
        previousBasementMode_ = CameraMode::FirstPersonWalk;
        break;
    case CameraMode::VehicleCockpit:
        activeStrategy_ = vehicleCockpit_.get();
        previousBasementMode_ = CameraMode::VehicleCockpit;
        break;
    case CameraMode::VehicleChase:
        activeStrategy_ = vehicleChase_.get();
        previousBasementMode_ = CameraMode::VehicleChase;
        break;
    case CameraMode::BasementCctv:
        activeStrategy_ = basementCctv_.get();
        previousBasementMode_ = CameraMode::BasementCctv;
        break;
    case CameraMode::TopDown:
        activeStrategy_ = topDown_.get();
        break;
    }
    if (activeStrategy_) {
        activeStrategy_->enter(ctx);
    }
}

void Camera::cycleBasementMode(const CameraContext& ctx) {
    CameraMode nextMode = CameraMode::FirstPersonWalk;
    switch (currentMode_) {
    case CameraMode::BasementOrbit:
        nextMode = CameraMode::FirstPersonWalk;
        break;
    case CameraMode::FirstPersonWalk:
        nextMode = CameraMode::VehicleCockpit;
        break;
    case CameraMode::VehicleCockpit:
        nextMode = CameraMode::VehicleChase;
        break;
    case CameraMode::VehicleChase:
        nextMode = CameraMode::BasementCctv;
        break;
    case CameraMode::BasementCctv:
        nextMode = CameraMode::BasementOrbit;
        break;
    default:
        nextMode = CameraMode::FirstPersonWalk;
        break;
    }
    setMode(nextMode, ctx);
}

void Camera::cycleCctvChannel() {
    if (currentMode_ == CameraMode::BasementCctv) {
        basementCctv_->nextChannel();
    } else {
        // Tự động chuyển sang chế độ CCTV nếu chưa bật
        CameraContext dummy;
        setMode(CameraMode::BasementCctv, dummy);
    }
}

void Camera::focusExterior() {
    CameraContext ctx;
    ctx.floorY = AeonLayout::GROUND_LEVEL_Y_M;
    setMode(CameraMode::ExteriorOrbit, ctx);
    topDown_->setHeight(AeonLayout::CAMERA_TOP_VIEW_HEIGHT_M);
    topDown_->setTarget(glm::vec3(AeonLayout::MALL_CAMERA_TARGET_X_M,
                                  AeonLayout::MALL_CAMERA_TARGET_Y_M,
                                  AeonLayout::WORLD_ORIGIN_Z_M));
}

void Camera::focusBasement(float floorY) {
    CameraContext ctx;
    ctx.floorY = floorY;
    const float targetY = floorY + AeonLayout::FLOOR_TO_CEILING_M *
                                   AeonLayout::CAMERA_TARGET_HEIGHT_FACTOR;
    basementOrbit_->setTarget(glm::vec3(AeonLayout::CAMERA_BASEMENT_TARGET_X_M,
                                        targetY,
                                        AeonLayout::CAMERA_BASEMENT_TARGET_Z_M));
    firstPersonWalk_->setFloorY(floorY);
    topDown_->setHeight(AeonLayout::CAMERA_BASEMENT_TOP_VIEW_HEIGHT_M);
    topDown_->setTarget(glm::vec3(AeonLayout::CAMERA_BASEMENT_TARGET_X_M,
                                  targetY,
                                  AeonLayout::CAMERA_BASEMENT_TARGET_Z_M));

    // Khôi phục chế độ hầm gần nhất hoặc mặc định là FirstPersonWalk để trải nghiệm chân thực!
    if (previousBasementMode_ != CameraMode::ExteriorOrbit) {
        setMode(previousBasementMode_, ctx);
    } else {
        setMode(CameraMode::FirstPersonWalk, ctx);
    }
}

void Camera::toggleTopView() {
    CameraContext ctx;
    if (currentMode_ == CameraMode::TopDown) {
        setMode(previousBasementMode_, ctx);
    } else {
        setMode(CameraMode::TopDown, ctx);
    }
}

void Camera::setTargetY(float targetY) {
    if (basementOrbit_) {
        glm::vec3 t = basementOrbit_->getTarget();
        t.y = targetY;
        basementOrbit_->setTarget(t);
    }
}

void Camera::update(const CameraContext& ctx) {
    if (activeStrategy_) {
        activeStrategy_->update(ctx);
    }
}

void Camera::orbit(float deltaX, float deltaY) {
    if (activeStrategy_) {
        activeStrategy_->onMouseOrbit(deltaX, deltaY);
    }
}

void Camera::zoom(double wheelDelta) {
    if (activeStrategy_) {
        activeStrategy_->onZoom(wheelDelta);
    }
}

glm::vec3 Camera::position() const {
    return activeStrategy_ ? activeStrategy_->getPosition() : glm::vec3(0.0f);
}

glm::vec3 Camera::target() const {
    return activeStrategy_ ? activeStrategy_->getTarget() : glm::vec3(0.0f, 0.0f, 1.0f);
}

glm::mat4 Camera::viewProjection(int width, int height) const {
    if (!activeStrategy_) return glm::mat4(1.0f);

    const glm::vec3 pos = activeStrategy_->getPosition();
    const glm::vec3 tgt = activeStrategy_->getTarget();
    const glm::vec3 up = activeStrategy_->getUp();
    const float fov = activeStrategy_->getFovDeg();

    const glm::mat4 view = glm::lookAt(pos, tgt, up);
    const glm::mat4 projection = glm::perspective(
        glm::radians(fov),
        static_cast<float>(width) / static_cast<float>(height),
        AeonLayout::CAMERA_NEAR_CLIP_M,
        AeonLayout::CAMERA_FAR_CLIP_M);
    return projection * view;
}

const char* Camera::currentModeName() const {
    return activeStrategy_ ? activeStrategy_->getName() : "CAMERA";
}

const char* Camera::currentModeHint() const {
    return activeStrategy_ ? activeStrategy_->getHint() : "";
}
