#pragma once

// ============================================================================
//  camera.h - Object-Oriented Camera System for Aeon Mall Parking Simulation.
//  Provides realistic basement inspection modes:
//   - First-Person Pedestrian Walk (eye-level inspection with head-bobbing)
//   - Vehicle Cockpit View (driver's seat perspective)
//   - Vehicle Chase View (dynamic third-person follow cam)
//   - Basement CCTV Surveillance (multi-channel security monitoring)
//   - Orbit & Top-Down Inspection Views
// ============================================================================

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "aeon_layout.h"

// Context passed from the simulation world each frame to the camera system.
struct CameraContext {
    float dt = 0.016f;
    float floorY = AeonLayout::FLOOR_HEIGHT_B1_M;
    float upperFloorY = AeonLayout::GROUND_LEVEL_Y_M;
    int floorIndex = AeonLayout::FLOOR_INDEX_B1;

    // Vehicle tracking data (from manual driver or active AI vehicle)
    bool hasVehicle = false;
    glm::vec3 vehiclePosition{0.0f};
    float vehicleYaw = 0.0f;
    float vehicleSpeed = 0.0f;

    // Pedestrian walk inputs (when in First-Person Walk mode)
    float moveForward = 0.0f;   // +1 = forward, -1 = backward
    float moveRight = 0.0f;     // +1 = strafe right, -1 = strafe left
    bool isSprinting = false;   // sprint multiplier
};

// Available camera perspectives
enum class CameraMode {
    ExteriorOrbit,       // Toàn cảnh bên ngoài trung tâm thương mại AEON
    BasementOrbit,       // Sa bàn góc nhìn cao toàn cảnh tầng hầm
    FirstPersonWalk,     // Góc nhìn thứ nhất: Đi bộ dạo trong hầm ở tầm mắt người thật (~1.65m)
    VehicleCockpit,      // Góc nhìn buồng lái: Ngồi sau vô lăng trong xe
    VehicleChase,        // Góc nhìn bám đuôi: Đuổi theo xe từ phía sau
    BasementCctv,        // Camera an ninh giám sát gắn trần tầng hầm
    TopDown              // Góc nhìn sơ đồ trực giao từ trên nóc hầm xuống
};

// ============================================================================
//  OOP Interface: ICameraStrategy (Abstract Base Class)
// ============================================================================
class ICameraStrategy {
public:
    virtual ~ICameraStrategy() = default;

    virtual void enter(const CameraContext& ctx) { (void)ctx; }
    virtual void update(const CameraContext& ctx) = 0;
    virtual glm::vec3 getPosition() const = 0;
    virtual glm::vec3 getTarget() const = 0;
    virtual glm::vec3 getUp() const { return glm::vec3(0.0f, 1.0f, 0.0f); }
    virtual float getFovDeg() const { return AeonLayout::CAMERA_FOV_DEG; }

    virtual void onMouseOrbit(float deltaX, float deltaY) { (void)deltaX; (void)deltaY; }
    virtual void onZoom(double wheelDelta) { (void)wheelDelta; }

    virtual const char* getName() const = 0;
    virtual const char* getHint() const = 0;
};

// ============================================================================
//  Concrete Strategy: OrbitView (Exterior & Basement Bird's-Eye)
// ============================================================================
class OrbitView : public ICameraStrategy {
public:
    OrbitView(float yaw, float pitch, float distance,
              const glm::vec3& target, const char* name, const char* hint);

    void update(const CameraContext& ctx) override;
    glm::vec3 getPosition() const override;
    glm::vec3 getTarget() const override { return target_; }
    void onMouseOrbit(float deltaX, float deltaY) override;
    void onZoom(double wheelDelta) override;

    const char* getName() const override { return name_; }
    const char* getHint() const override { return hint_; }

    void setTarget(const glm::vec3& target) { target_ = target; }
    void setDistance(float dist) { distance_ = dist; }
    void setAngles(float yaw, float pitch) { yaw_ = yaw; pitch_ = pitch; }

private:
    float yaw_;
    float pitch_;
    float distance_;
    glm::vec3 target_;
    const char* name_;
    const char* hint_;
};

// ============================================================================
//  Concrete Strategy: FirstPersonWalkView (Realistic Basement Walk)
// ============================================================================
class FirstPersonWalkView : public ICameraStrategy {
public:
    FirstPersonWalkView();

    void enter(const CameraContext& ctx) override;
    void update(const CameraContext& ctx) override;
    glm::vec3 getPosition() const override;
    glm::vec3 getTarget() const override;
    void onMouseOrbit(float deltaX, float deltaY) override;

    const char* getName() const override { return "FIRST-PERSON WALK (DI BO TRONG HAM)"; }
    const char* getHint() const override { return "WASD / ARROWS: DI CHUYEN | SHIFT: CHAY | CHUOT: XOAY NHIN"; }

    void setFloorY(float floorY);

private:
    glm::vec3 position_{0.0f};
    float yaw_ = -1.57f;          // Facing positive X / center aisle
    float pitch_ = -0.05f;        // Slightly looking forward-down
    float currentFloorY_ = AeonLayout::FLOOR_HEIGHT_B1_M;
    float eyeHeight_ = 1.65f;     // Tầm mắt người tiêu chuẩn 1.65m
    float bobTimer_ = 0.0f;
    float bobOffset_ = 0.0f;
    bool isMoving_ = false;

    void clampWithinBasement();
};

// ============================================================================
//  Concrete Strategy: VehicleCockpitView (Driver Seat View)
// ============================================================================
class VehicleCockpitView : public ICameraStrategy {
public:
    VehicleCockpitView();

    void update(const CameraContext& ctx) override;
    glm::vec3 getPosition() const override;
    glm::vec3 getTarget() const override;
    float getFovDeg() const override { return 72.0f; } // Wide cabin view

    const char* getName() const override { return "CABIN COCKPIT (GOC NHIN TAI XE)"; }
    const char* getHint() const override { return "M: BAT LAI XE THU CONG | WASD: LAI XE TRONG HAM"; }

private:
    glm::vec3 currentPos_{0.0f};
    glm::vec3 currentTarget_{0.0f, 0.0f, 1.0f};
    float smoothYaw_ = 0.0f;
};

// ============================================================================
//  Concrete Strategy: VehicleChaseView (Third-Person Follow Cam)
// ============================================================================
class VehicleChaseView : public ICameraStrategy {
public:
    VehicleChaseView();

    void update(const CameraContext& ctx) override;
    glm::vec3 getPosition() const override;
    glm::vec3 getTarget() const override;

    const char* getName() const override { return "VEHICLE CHASE (BAM DUOI XE)"; }
    const char* getHint() const override { return "M: BAT LAI XE THU CONG | THEO DOI XE TRONG HAM"; }

private:
    glm::vec3 currentPos_{0.0f};
    glm::vec3 currentTarget_{0.0f};
    float smoothYaw_ = 0.0f;
};

// ============================================================================
//  Concrete Strategy: BasementCctvView (Multi-Channel Security Surveillance)
// ============================================================================
struct CctvChannel {
    std::string name;
    glm::vec3 ceilingOffset; // Offset from basement origin or floor
    glm::vec3 lookAtTarget;
    float sweepAmplitudeRad = 0.12f; // Tự động xoay quét an ninh
    float sweepSpeed = 0.45f;
};

class BasementCctvView : public ICameraStrategy {
public:
    BasementCctvView();

    void enter(const CameraContext& ctx) override;
    void update(const CameraContext& ctx) override;
    glm::vec3 getPosition() const override;
    glm::vec3 getTarget() const override;
    float getFovDeg() const override { return 78.0f; } // Wide CCTV camera lens

    const char* getName() const override;
    const char* getHint() const override { return "K HOAC 3: DOI KENH CAMERA AN NINH (1 DEN 5)"; }

    void nextChannel();
    void prevChannel();
    int currentChannel() const { return currentChannel_; }

private:
    std::vector<CctvChannel> channels_;
    int currentChannel_ = 0;
    float sweepTimer_ = 0.0f;
    float currentFloorY_ = AeonLayout::FLOOR_HEIGHT_B1_M;
    mutable std::string currentDisplayName_;

    void setupChannels();
};

// ============================================================================
//  Concrete Strategy: TopDownView (Orthographic Plan Inspection)
// ============================================================================
class TopDownView : public ICameraStrategy {
public:
    TopDownView();

    void update(const CameraContext& ctx) override;
    glm::vec3 getPosition() const override;
    glm::vec3 getTarget() const override;
    glm::vec3 getUp() const override { return glm::vec3(0.0f, 0.0f, -1.0f); }

    const char* getName() const override { return "TOP-DOWN MAP (SO DO MAT BANG)"; }
    const char* getHint() const override { return "T HOAC C: TRO LAI GOC NHIN 3D"; }

    void setHeight(float height) { height_ = height; }
    void setTarget(const glm::vec3& target) { target_ = target; }

private:
    glm::vec3 target_{AeonLayout::SCENE_CENTER_X_M, AeonLayout::GROUND_LEVEL_Y_M, AeonLayout::WORLD_ORIGIN_Z_M};
    float height_ = AeonLayout::CAMERA_TOP_VIEW_HEIGHT_M;
};

// ============================================================================
//  Main Facade Class: Camera (Object-Oriented Controller)
// ============================================================================
class Camera {
public:
    Camera();
    ~Camera();

    // Mode switching
    void setMode(CameraMode mode, const CameraContext& ctx);
    CameraMode mode() const { return currentMode_; }
    void cycleBasementMode(const CameraContext& ctx);
    void cycleCctvChannel();

    // Presets for compatibility with existing code
    void focusExterior();
    void focusBasement(float floorY = AeonLayout::FLOOR_HEIGHT_B1_M);
    void toggleTopView();
    bool topView() const { return currentMode_ == CameraMode::TopDown; }
    void setTargetY(float targetY);

    // Frame update & mouse interaction
    void update(const CameraContext& ctx);
    void orbit(float deltaX, float deltaY);
    void zoom(double wheelDelta);

    // Output transformation matrices
    glm::vec3 position() const;
    glm::vec3 target() const;
    glm::mat4 viewProjection(int width, int height) const;

    // UI information
    const char* currentModeName() const;
    const char* currentModeHint() const;

private:
    CameraMode currentMode_ = CameraMode::ExteriorOrbit;
    CameraMode previousBasementMode_ = CameraMode::BasementOrbit;

    // OOP Polymorphic Strategies
    std::unique_ptr<OrbitView> exteriorOrbit_;
    std::unique_ptr<OrbitView> basementOrbit_;
    std::unique_ptr<FirstPersonWalkView> firstPersonWalk_;
    std::unique_ptr<VehicleCockpitView> vehicleCockpit_;
    std::unique_ptr<VehicleChaseView> vehicleChase_;
    std::unique_ptr<BasementCctvView> basementCctv_;
    std::unique_ptr<TopDownView> topDown_;

    ICameraStrategy* activeStrategy_ = nullptr;
};
