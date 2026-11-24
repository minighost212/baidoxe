#pragma once

// ============================================================================
//  guard_station.h - Security checkpoint & barrier control booth at ramp foot.
//  Encapsulated OOP design modeling the realistic basement entrance station:
//  - Raised traffic island with alternating yellow/black hazard perimeter
//  - Central guard workstation: desk, computer case, dual LCD monitors, chair
//  - Dual high-visibility orange barrier housings with red/white boom arms
//  - Rubber speed bumps on inbound and outbound lanes
//  - Overhead license plate recognition (LPR) cameras suspended from ceiling
//  - Pavement directional arrows
// ============================================================================

#include <glm/glm.hpp>
#include "renderer.h"

class GuardStation {
public:
    explicit GuardStation(Renderer& renderer);

    void render(float entryBarrierTimer, float exitBarrierTimer, float floorY) const;

    glm::vec3 center() const { return center_; }
    float entryGateX() const { return entryBarrierX_; }
    float exitGateX() const { return exitBarrierX_; }
    float islandLength() const { return islandLength_; }
    float islandWidth() const { return islandWidth_; }

private:
    void drawRaisedIsland(float floorY) const;
    void drawGuardDesk(float floorY) const;
    void drawDualMonitors(float floorY) const;
    void drawGuardChair(float floorY) const;
    void drawBarrierGate(const glm::vec3& postPos, float armLength, float openTimer, bool isEntry) const;
    void drawSpeedBumps(float floorY) const;
    void drawOverheadCameras(float floorY) const;
    void drawLaneDirectionArrows(float floorY) const;

    Renderer& renderer_;
    glm::vec3 center_;
    float islandLength_ = 4.20f;
    float islandWidth_  = 0.95f;
    float islandHeight_ = 0.20f;
    float entryBarrierX_;
    float exitBarrierX_;
};
