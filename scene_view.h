#pragma once

// ============================================================================
//  scene_view.h - presentation layer.
//  SceneRenderer draws the world geometry (shell, columns, bays, ramp,
//  barriers, cars) and Hud draws the orthographic information panel.
// ============================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "aeon_layout.h"
#include "camera.h"
#include "guard_station.h"
#include "obj_model.h"
#include "parking_world.h"
#include "renderer.h"

class BasementRamp {
public:
    explicit BasementRamp(Renderer& renderer) : renderer_(renderer) {}

    void render(float upperLevelY, float lowerLevelY, bool groundEntrance) const;

private:
    void drawDeck(float upperLevelY, float lowerLevelY) const;
    void drawRetainingWalls(float upperLevelY, float lowerLevelY) const;
    void drawSafetyMarkings(float upperLevelY, float lowerLevelY) const;
    void drawEntrancePortal(float upperLevelY) const;

    Renderer& renderer_;
};

class ParkingGate {
public:
    ParkingGate(Renderer& renderer, float laneCenterZ, bool boothOnPositiveZ)
        : renderer_(renderer), laneCenterZ_(laneCenterZ),
          boothOnPositiveZ_(boothOnPositiveZ) {}

    void render(float openTimer, float upperLevelY, float lowerLevelY) const;

private:
    Renderer& renderer_;
    float laneCenterZ_ = AeonLayout::WORLD_ORIGIN_Z_M;
    bool boothOnPositiveZ_ = false;
};

class AeonMallExterior {
public:
    explicit AeonMallExterior(Renderer& renderer) : renderer_(renderer) {}

    bool initialise();  // Load OBJ building model
    void render() const;

private:
    void drawGround() const;
    void drawStreet() const;
    void drawParkingEntrance() const;
    void drawNeighborhood() const;
    void drawTree(float x, float z, float scale, const glm::vec3& foliage,
                  float baseY = 0.0f) const;
    void drawContextBuilding(float x, float z, float width, float depth,
                             float height, int floors,
                             const glm::vec3& facade) const;

    Renderer& renderer_;
    ObjModel buildingModel_;
    glm::mat4 modelMatrix_{1.0f};
};

class SceneRenderer {
public:
    explicit SceneRenderer(Renderer& renderer)
        : renderer_(renderer), mallExterior_(renderer), ramp_(renderer),
          guardStation_(renderer) {}

    bool initialise() { return mallExterior_.initialise(); }
    void render(const ParkingWorld& world, const ElevatorSystem& elevator, bool topView = false);
    void toggleExterior() { exteriorVisible_ = !exteriorVisible_; }
    void setExteriorVisible(bool visible) { exteriorVisible_ = visible; }
    bool exteriorVisible() const { return exteriorVisible_; }
    void toggleCeiling() { ceilingVisible_ = !ceilingVisible_; }
    void setCeilingVisible(bool visible) { ceilingVisible_ = visible; }
    bool ceilingVisible() const { return ceilingVisible_; }

private:
    void drawShell(float floorY);
    void drawBasementCeiling(float floorY, const std::vector<ParkingSlot>& slots);
    void drawRealisticColumns(const ParkingWorld& world, float floorY);
    void drawLaneMarkings(float floorY);
    void drawArrow(float x, float z, float yaw, float floorY);
    void drawParkingSlot(const ParkingSlot& slot, float floorY);
    void drawLightFixtures(float floorY);
    void drawMotorbikeArea(float floorY);
    void drawMotorbike(const ParkedMotorbike& bike);
    void drawElevatorCore(float floorY, const ElevatorSystem& elevator);

    Renderer& renderer_;
    AeonMallExterior mallExterior_;
    BasementRamp ramp_;
    GuardStation guardStation_;
    bool exteriorVisible_ = true;
    bool ceilingVisible_ = true;
};

inline bool AeonMallExterior::initialise() {
    const std::vector<std::string> exclude = {"ground", "walk"};
    if (!buildingModel_.load("model/aeon_danang_v3.obj", exclude)) return false;
    buildingModel_.upload();
    const glm::vec3 bmin = buildingModel_.boundingBoxMin();
    const glm::vec3 bmax = buildingModel_.boundingBoxMax();
    const glm::vec3 center((bmin.x + bmax.x) * 0.5f, bmin.y,
                           (bmin.z + bmax.z) * 0.5f);
    constexpr float kScale = AeonLayout::MALL_OBJ_SCALE_M;
    constexpr float kLift = AeonLayout::MALL_OBJ_Y_LIFT_M;
    modelMatrix_ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, kLift, 0.0f)) *
                   glm::scale(glm::mat4(1.0f), glm::vec3(kScale)) *
                   glm::translate(glm::mat4(1.0f), -center);
    return true;
}

inline void AeonMallExterior::render() const {
    drawGround();
    drawStreet();
    buildingModel_.render(renderer_, modelMatrix_);
    drawNeighborhood();
}

inline void AeonMallExterior::drawGround() const {
    const float minX = AeonLayout::RAMP_START_X_M -
        AeonLayout::RAMP_APRON_LENGTH_M * AeonLayout::GEOMETRY_HALF_RATIO -
        AeonLayout::EXTERIOR_GROUND_MARGIN_M;
    const float maxX = AeonLayout::MALL_STREET_LENGTH_X_M *
        AeonLayout::GEOMETRY_HALF_RATIO + AeonLayout::EXTERIOR_GROUND_MARGIN_M;
    const float minZ = AeonLayout::MALL_STREET_CENTER_Z_M -
        AeonLayout::MALL_STREET_WIDTH_Z_M * AeonLayout::GEOMETRY_HALF_RATIO -
        AeonLayout::EXTERIOR_GROUND_MARGIN_M;
    const float maxZ = AeonLayout::MALL_PODIUM_WIDTH_Z_M *
        AeonLayout::GEOMETRY_HALF_RATIO + AeonLayout::EXTERIOR_GROUND_MARGIN_M;
    const float apronStartX = AeonLayout::RAMP_START_X_M -
        AeonLayout::RAMP_APRON_LENGTH_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float rampSideClearanceZ =
        AeonLayout::RAMP_TOTAL_WIDTH_Z_M * AeonLayout::GEOMETRY_HALF_RATIO +
        AeonLayout::RAMP_RETAINING_WALL_THICKNESS_M +
        AeonLayout::RAMP_RETAINING_CAP_OVERHANG_M *
            AeonLayout::GEOMETRY_HALF_RATIO +
        AeonLayout::EXTERIOR_RAMP_CLEARANCE_M;
    const float groundBottomY = AeonLayout::GROUND_LEVEL_Y_M -
        AeonLayout::EXTERIOR_GROUND_THICKNESS_M -
        AeonLayout::EXTERIOR_GROUND_SINK_M;
    const glm::vec3 groundColor(0.25f, 0.33f, 0.26f);

    const auto drawGroundPatch = [&](float x0, float x1, float z0, float z1) {
        if (x1 <= x0 || z1 <= z0) return;
        renderer_.drawBox(
            glm::vec3((x0 + x1) * AeonLayout::GEOMETRY_HALF_RATIO, groundBottomY,
                      (z0 + z1) * AeonLayout::GEOMETRY_HALF_RATIO),
            glm::vec3(x1 - x0, AeonLayout::EXTERIOR_GROUND_THICKNESS_M,
                      z1 - z0),
            groundColor);
    };

    // Split the terrain around the descending ramp so its opening stays clear.
    drawGroundPatch(minX, apronStartX, minZ, maxZ);
    drawGroundPatch(apronStartX, AeonLayout::RAMP_END_X_M,
                    minZ, -rampSideClearanceZ);
    drawGroundPatch(apronStartX, AeonLayout::RAMP_END_X_M,
                    rampSideClearanceZ, maxZ);
    drawGroundPatch(AeonLayout::RAMP_END_X_M, maxX, minZ, maxZ);
}

inline void AeonMallExterior::drawStreet() const {
    const float frontZ = AeonLayout::MALL_OBJ_FRONT_Z_M;
    const glm::vec3 asphalt(0.15f, 0.17f, 0.18f);
    const glm::vec3 sidewalk(0.62f, 0.62f, 0.59f);

    renderer_.drawBox(
        glm::vec3(AeonLayout::WORLD_ORIGIN_X_M, -AeonLayout::STREET_ASPHALT_THICKNESS_M,
                  AeonLayout::MALL_STREET_CENTER_Z_M),
        glm::vec3(AeonLayout::MALL_STREET_LENGTH_X_M, AeonLayout::STREET_ASPHALT_THICKNESS_M,
                  AeonLayout::MALL_STREET_WIDTH_Z_M), asphalt);
    renderer_.drawBox(
        glm::vec3(AeonLayout::WORLD_ORIGIN_X_M, AeonLayout::GROUND_LEVEL_Y_M,
                  frontZ - AeonLayout::MALL_SIDEWALK_WIDTH_Z_M * AeonLayout::GEOMETRY_HALF_RATIO),
        glm::vec3(AeonLayout::MALL_PODIUM_LENGTH_X_M + AeonLayout::SIDEWALK_EXTRA_LENGTH_X_M, AeonLayout::SIDEWALK_HEIGHT_M,
                  AeonLayout::MALL_SIDEWALK_WIDTH_Z_M), sidewalk);
    for (int dash = 0; dash < AeonLayout::MALL_STREET_DASH_COUNT; ++dash) {
        const float x = -AeonLayout::MALL_STREET_LENGTH_X_M * AeonLayout::GEOMETRY_HALF_RATIO + AeonLayout::STREET_DASH_START_OFFSET_M +
                        static_cast<float>(dash) * AeonLayout::MALL_STREET_DASH_SPACING_M;
        renderer_.drawBox(
            glm::vec3(x, AeonLayout::GROUND_LEVEL_Y_M, AeonLayout::MALL_STREET_CENTER_Z_M),
            glm::vec3(AeonLayout::MALL_STREET_DASH_LENGTH_M, AeonLayout::STREET_MARKING_THICKNESS_M,
                      AeonLayout::MALL_STREET_MARKING_WIDTH_M),
            glm::vec3(0.86f, 0.85f, 0.77f));
    }

    for (int tree = 0; tree < AeonLayout::MALL_STREET_TREE_COUNT; ++tree) {
        const float x =
            (static_cast<float>(tree) -
             static_cast<float>(AeonLayout::MALL_STREET_TREE_COUNT - 1) * AeonLayout::GEOMETRY_HALF_RATIO) *
            AeonLayout::MALL_STREET_TREE_SPACING_M;
        renderer_.drawBox(
            glm::vec3(x, 0.0f, AeonLayout::MALL_STREET_TREE_LINE_Z_M),
            glm::vec3(4.7f, 0.10f, 2.3f), glm::vec3(0.20f, 0.32f, 0.19f));
        renderer_.drawBox(
            glm::vec3(x, 0.10f, AeonLayout::MALL_STREET_TREE_LINE_Z_M),
            glm::vec3(AeonLayout::MALL_STREET_TREE_TRUNK_WIDTH_M,
                      AeonLayout::MALL_STREET_TREE_TRUNK_HEIGHT_M,
                      AeonLayout::MALL_STREET_TREE_TRUNK_WIDTH_M),
            glm::vec3(0.31f, 0.20f, 0.12f));
        renderer_.drawBox(
            glm::vec3(x, AeonLayout::TREE_LOWER_CANOPY_CENTER_Y_M + 0.10f,
                      AeonLayout::MALL_STREET_TREE_LINE_Z_M),
            glm::vec3(AeonLayout::MALL_STREET_TREE_CANOPY_WIDTH_M,
                      AeonLayout::MALL_STREET_TREE_CANOPY_HEIGHT_M,
                      AeonLayout::MALL_STREET_TREE_CANOPY_DEPTH_M),
            glm::vec3(0.15f, 0.34f, 0.19f));
        renderer_.drawBox(
            glm::vec3(x, AeonLayout::TREE_UPPER_CANOPY_CENTER_Y_M + 0.10f,
                      AeonLayout::MALL_STREET_TREE_LINE_Z_M),
            glm::vec3(AeonLayout::MALL_STREET_TREE_CANOPY_WIDTH_M * AeonLayout::TREE_UPPER_CANOPY_WIDTH_RATIO,
                      AeonLayout::MALL_STREET_TREE_CANOPY_HEIGHT_M * AeonLayout::TREE_UPPER_CANOPY_HEIGHT_RATIO,
                      AeonLayout::MALL_STREET_TREE_CANOPY_DEPTH_M * AeonLayout::TREE_UPPER_CANOPY_WIDTH_RATIO),
            glm::vec3(0.20f, 0.41f, 0.22f));
    }
}

inline void AeonMallExterior::drawParkingEntrance() const {
    // Centre the garage entrance on the mall frontage (X = 0) so it lines
    // up with the main public entry, matching the original facade layout.
    constexpr float entranceX = 0.0f;
    constexpr float rampStartZ = -35.2f;
    constexpr float portalZ = AeonLayout::MALL_OBJ_FRONT_Z_M - 0.42f;
    constexpr float rampLength = portalZ - rampStartZ;
    constexpr float rampWidth = 6.2f;
    constexpr float rampDrop = 1.1f;
    constexpr float slabThickness = 0.22f;
    constexpr float wallHeight = 1.35f;
    constexpr float wallThickness = 0.30f;
    constexpr float rampMidZ = (rampStartZ + portalZ) * 0.5f;
    constexpr float rampMidSurfaceY = -rampDrop * 0.5f;
    const float rampPitch = std::atan(rampDrop / rampLength);

    const auto drawSlopedBox = [&](const glm::vec3& center,
                                   const glm::vec3& size,
                                   const glm::vec3& color) {
        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), center) *
            glm::rotate(glm::mat4(1.0f), rampPitch, glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::translate(glm::mat4(1.0f),
                           glm::vec3(0.0f, -size.y * 0.5f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), size);
        renderer_.draw(renderer_.boxMesh(), model, color);
    };

    const glm::vec3 concrete(0.56f, 0.58f, 0.56f);
    drawSlopedBox(
        glm::vec3(entranceX, rampMidSurfaceY - slabThickness * 0.5f, rampMidZ),
        glm::vec3(rampWidth, slabThickness, rampLength),
        glm::vec3(0.18f, 0.20f, 0.21f));

    for (const float side : {-1.0f, 1.0f}) {
        drawSlopedBox(
            glm::vec3(entranceX + side * (rampWidth * 0.5f +
                                          wallThickness * 0.5f),
                      rampMidSurfaceY + wallHeight * 0.5f, rampMidZ),
            glm::vec3(wallThickness, wallHeight, rampLength),
            concrete);
        drawSlopedBox(
            glm::vec3(entranceX + side * (rampWidth * 0.5f - 0.16f),
                      rampMidSurfaceY + 0.02f, rampMidZ),
            glm::vec3(0.10f, 0.04f, rampLength - 0.4f),
            glm::vec3(0.94f, 0.73f, 0.18f));
    }

    constexpr float portalBaseY = -rampDrop;
    constexpr float openingWidth = 5.0f;
    constexpr float openingHeight = 3.35f;
    constexpr float pierWidth = 0.72f;
    const glm::vec3 darkOpening(0.035f, 0.045f, 0.052f);
    renderer_.drawBox(
        glm::vec3(entranceX, portalBaseY, portalZ),
        glm::vec3(openingWidth, openingHeight, 0.22f),
        darkOpening);
    for (const float side : {-1.0f, 1.0f}) {
        renderer_.drawBox(
            glm::vec3(entranceX + side * (openingWidth * 0.5f +
                                          pierWidth * 0.5f),
                      portalBaseY, portalZ - 0.20f),
            glm::vec3(pierWidth, openingHeight + 0.45f, 0.70f),
            concrete);
    }
    renderer_.drawBox(
        glm::vec3(entranceX, portalBaseY + openingHeight,
                  portalZ - 0.20f),
        glm::vec3(openingWidth + pierWidth * 2.0f, 0.58f, 0.70f),
        concrete);
    renderer_.drawBox(
        glm::vec3(entranceX, portalBaseY + openingHeight + 0.58f,
                  portalZ - 0.58f),
        glm::vec3(openingWidth + 1.4f, 0.24f, 1.35f),
        glm::vec3(0.12f, 0.29f, 0.43f));
}

inline void AeonMallExterior::drawNeighborhood() const {
    constexpr float roadThickness = AeonLayout::STREET_ASPHALT_THICKNESS_M;
    constexpr float sideRoadX = 31.5f;
    constexpr float sideRoadWidth = 8.0f;
    constexpr float sideRoadMinZ = -52.0f;
    constexpr float sideRoadMaxZ = 37.0f;
    constexpr glm::vec3 asphalt(0.13f, 0.15f, 0.16f);
    constexpr glm::vec3 curb(0.55f, 0.57f, 0.54f);
    constexpr glm::vec3 sidewalk(0.62f, 0.63f, 0.59f);
    constexpr glm::vec3 grass(0.20f, 0.34f, 0.22f);
    constexpr glm::vec3 laneMark(0.88f, 0.85f, 0.72f);

    // A side avenue joins the frontage road at the east end of the mall.
    renderer_.drawBox(
        glm::vec3(sideRoadX, -roadThickness, (sideRoadMinZ + sideRoadMaxZ) * 0.5f),
        glm::vec3(sideRoadWidth, roadThickness, sideRoadMaxZ - sideRoadMinZ),
        asphalt);
    renderer_.drawBox(
        glm::vec3(sideRoadX - sideRoadWidth * 0.5f - 0.75f, 0.0f,
                  (sideRoadMinZ + sideRoadMaxZ) * 0.5f),
        glm::vec3(1.5f, AeonLayout::SIDEWALK_HEIGHT_M,
                  sideRoadMaxZ - sideRoadMinZ),
        sidewalk);
    renderer_.drawBox(
        glm::vec3(sideRoadX + sideRoadWidth * 0.5f + 0.75f, 0.0f,
                  (sideRoadMinZ + sideRoadMaxZ) * 0.5f),
        glm::vec3(1.5f, AeonLayout::SIDEWALK_HEIGHT_M,
                  sideRoadMaxZ - sideRoadMinZ),
        sidewalk);

    // Curb and sidewalk behind the frontage boulevard.
    renderer_.drawBox(
        glm::vec3(0.0f, 0.0f, -42.0f),
        glm::vec3(78.0f, AeonLayout::SIDEWALK_HEIGHT_M, 1.8f), sidewalk);
    renderer_.drawBox(glm::vec3(0.0f, 0.0f, -43.05f),
        glm::vec3(78.0f, 0.22f, 0.22f), curb);
    renderer_.drawBox(
        glm::vec3(sideRoadX, 0.0f, -23.2f),
        glm::vec3(7.2f, 0.035f, 0.22f), laneMark);

    // Mark the side avenue and its pedestrian crossing.
    for (int dash = 0; dash < 9; ++dash) {
        const float z = sideRoadMinZ + 5.0f + static_cast<float>(dash) * 9.0f;
        if (z > AeonLayout::MALL_STREET_CENTER_Z_M -
                    AeonLayout::MALL_STREET_WIDTH_Z_M * 0.5f - 1.0f &&
            z < AeonLayout::MALL_STREET_CENTER_Z_M +
                    AeonLayout::MALL_STREET_WIDTH_Z_M * 0.5f + 1.0f) {
            continue;
        }
        renderer_.drawBox(glm::vec3(sideRoadX, 0.0f, z),
            glm::vec3(0.12f, 0.025f, 3.2f), laneMark);
    }
    for (int stripe = 0; stripe < 9; ++stripe) {
        const float z = -27.0f + static_cast<float>(stripe) * 0.78f;
        renderer_.drawBox(glm::vec3(sideRoadX, 0.0f, z),
            glm::vec3(6.8f, 0.03f, 0.38f), laneMark);
    }

    // Landscaped buffers soften the large paved site and frame the ramp.
    for (const float x : {-22.0f, 22.0f}) {
        renderer_.drawBox(glm::vec3(x, 0.0f, 29.0f),
            glm::vec3(16.0f, 0.12f, 3.2f), grass);
    }
    for (const float z : {-11.0f, 11.0f}) {
        renderer_.drawBox(glm::vec3(-34.0f, 0.0f, z),
            glm::vec3(4.8f, 0.12f, 4.8f), grass);
    }
    for (const float z : {-19.0f, -5.0f, 9.0f, 23.0f}) {
        renderer_.drawBox(glm::vec3(38.6f, 0.0f, z),
            glm::vec3(3.2f, 0.12f, 9.0f), grass);
    }

    const std::array<glm::vec3, 3> foliageColors = {
        glm::vec3(0.13f, 0.34f, 0.18f),
        glm::vec3(0.18f, 0.39f, 0.20f),
        glm::vec3(0.22f, 0.40f, 0.18f)
    };
    for (int tree = 0; tree < 7; ++tree) {
        const float x = -24.0f + static_cast<float>(tree) * 8.0f;
        const glm::vec3 foliage = foliageColors[static_cast<std::size_t>(tree) % foliageColors.size()];
        drawTree(x, 29.0f, 0.82f, foliage, 0.12f);
        drawTree(x, -45.0f, 0.72f, foliage, 0.0f);
    }
    for (int tree = 0; tree < 4; ++tree) {
        const float z = -20.0f + static_cast<float>(tree) * 14.0f;
        const glm::vec3 foliage = foliageColors[static_cast<std::size_t>(tree) % foliageColors.size()];
        drawTree(38.6f, z, 0.78f, foliage, 0.12f);
    }
    drawTree(-34.0f, -11.0f, 0.78f, foliageColors[1], 0.12f);
    drawTree(-34.0f, 11.0f, 0.78f, foliageColors[2], 0.12f);

    // Low-rise neighbors give the mall a more believable urban setting.
    drawContextBuilding(-22.0f, 34.5f, 10.0f, 7.0f, 8.0f, 2,
                       glm::vec3(0.67f, 0.59f, 0.48f));
    drawContextBuilding(22.0f, 34.5f, 10.0f, 7.0f, 9.5f, 3,
                       glm::vec3(0.69f, 0.66f, 0.57f));
    drawContextBuilding(-21.0f, -50.0f, 12.0f, 7.0f, 9.0f, 3,
                       glm::vec3(0.65f, 0.57f, 0.49f));
    drawContextBuilding(-3.0f, -50.0f, 12.0f, 7.0f, 11.0f, 3,
                       glm::vec3(0.68f, 0.64f, 0.55f));
    drawContextBuilding(16.0f, -50.0f, 12.0f, 7.0f, 8.0f, 2,
                       glm::vec3(0.61f, 0.64f, 0.58f));
    drawContextBuilding(44.5f, -11.0f, 7.0f, 12.0f, 10.0f, 3,
                       glm::vec3(0.69f, 0.62f, 0.52f));
    drawContextBuilding(44.5f, 13.0f, 7.0f, 12.0f, 8.0f, 2,
                       glm::vec3(0.63f, 0.66f, 0.60f));
    // Simple pedestrian-scale street lamps along both roads.
    for (const float x : {-25.0f, -10.0f, 5.0f, 20.0f}) {
        renderer_.drawBox(glm::vec3(x, 0.0f, -42.8f),
            glm::vec3(0.18f, 6.0f, 0.18f), glm::vec3(0.22f, 0.24f, 0.24f));
        renderer_.drawBox(glm::vec3(x + 0.55f, 5.75f, -42.8f),
            glm::vec3(1.25f, 0.14f, 0.18f), glm::vec3(0.22f, 0.24f, 0.24f));
        renderer_.drawBox(glm::vec3(x + 1.05f, 5.60f, -42.8f),
            glm::vec3(0.65f, 0.18f, 0.48f), glm::vec3(0.95f, 0.78f, 0.48f));
    }
    for (const float z : {-36.0f, -12.0f, 12.0f, 32.0f}) {
        renderer_.drawBox(glm::vec3(36.1f, 0.0f, z),
            glm::vec3(0.18f, 6.0f, 0.18f), glm::vec3(0.22f, 0.24f, 0.24f));
        renderer_.drawBox(glm::vec3(36.1f, 5.75f, z + 0.55f),
            glm::vec3(0.18f, 0.14f, 1.25f), glm::vec3(0.22f, 0.24f, 0.24f));
        renderer_.drawBox(glm::vec3(36.1f, 5.60f, z + 1.05f),
            glm::vec3(0.48f, 0.18f, 0.65f), glm::vec3(0.95f, 0.78f, 0.48f));
    }
}

inline void AeonMallExterior::drawTree(float x, float z, float scale,
                                       const glm::vec3& foliage,
                                       float baseY) const {
    const glm::vec3 trunk(0.30f, 0.19f, 0.11f);
    renderer_.drawBox(glm::vec3(x, baseY, z),
        glm::vec3(0.34f * scale, 2.25f * scale, 0.34f * scale), trunk);
    renderer_.drawBox(glm::vec3(x, baseY + 1.45f * scale, z),
        glm::vec3(2.65f * scale, 1.75f * scale, 2.35f * scale), foliage);
    renderer_.drawBox(glm::vec3(x + 0.12f * scale, baseY + 2.45f * scale,
                                z - 0.06f * scale),
        glm::vec3(2.15f * scale, 1.55f * scale, 1.95f * scale),
        foliage * 1.12f);
    renderer_.drawBox(glm::vec3(x - 0.08f * scale, baseY + 3.35f * scale,
                                z + 0.06f * scale),
        glm::vec3(1.52f * scale, 1.35f * scale, 1.42f * scale),
        foliage * 0.92f);
}

inline void AeonMallExterior::drawContextBuilding(
    float x, float z, float width, float depth, float height, int floors,
    const glm::vec3& facade) const {
    const float floorHeight = height / static_cast<float>(floors);
    const float frontZ = z - depth * 0.5f - 0.08f;
    const int frontBays = std::max(2, static_cast<int>(width / 2.4f));
    const int sideBays = std::max(2, static_cast<int>(depth / 2.5f));
    const float frontBayWidth = width / static_cast<float>(frontBays);
    const float sideBayWidth = depth / static_cast<float>(sideBays);
    const glm::vec3 glass(0.17f, 0.31f, 0.39f);
    const glm::vec3 trim(0.82f, 0.78f, 0.68f);
    const glm::vec3 roof(0.34f, 0.36f, 0.34f);

    renderer_.drawBox(glm::vec3(x, 0.0f, z),
        glm::vec3(width, height, depth), facade);
    renderer_.drawBox(glm::vec3(x, height - 0.32f, z),
        glm::vec3(width + 0.35f, 0.32f, depth + 0.35f), roof);
    renderer_.drawBox(glm::vec3(x, 0.0f, frontZ - 1.0f),
        glm::vec3(width + 1.2f, 0.12f, 1.7f),
        glm::vec3(0.58f, 0.59f, 0.55f));

    for (int floor = 0; floor < floors; ++floor) {
        const float floorBase = static_cast<float>(floor) * floorHeight;
        const float windowBottom = floorBase + (floor == 0 ? 0.35f : 0.85f);
        const float windowHeight = std::min(
            floor == 0 ? 2.0f : 1.25f, floorHeight - (windowBottom - floorBase) - 0.30f);
        if (windowHeight <= 0.0f) continue;

        renderer_.drawBox(glm::vec3(x, floorBase, frontZ),
            glm::vec3(width + 0.08f, 0.12f, 0.15f), trim);
        for (int bay = 0; bay < frontBays; ++bay) {
            const float windowX = x - width * 0.5f +
                (static_cast<float>(bay) + 0.5f) * frontBayWidth;
            renderer_.drawBox(glm::vec3(windowX, windowBottom, frontZ - 0.04f),
                glm::vec3(frontBayWidth * 0.62f, windowHeight, 0.12f), glass);
        }
        for (int bay = 0; bay < sideBays; ++bay) {
            const float windowZ = z - depth * 0.5f +
                (static_cast<float>(bay) + 0.5f) * sideBayWidth;
            renderer_.drawBox(glm::vec3(x - width * 0.5f - 0.04f,
                                        windowBottom, windowZ),
                glm::vec3(0.12f, windowHeight, sideBayWidth * 0.58f), glass);
        }
    }
}


inline void BasementRamp::render(float upperLevelY, float lowerLevelY, bool groundEntrance) const {
    drawDeck(upperLevelY, lowerLevelY);
    drawRetainingWalls(upperLevelY, lowerLevelY);
    drawSafetyMarkings(upperLevelY, lowerLevelY);
    if (groundEntrance) drawEntrancePortal(upperLevelY);
}

inline void BasementRamp::drawDeck(float upperLevelY, float lowerLevelY) const {
    const float midY = (upperLevelY + lowerLevelY) * AeonLayout::GEOMETRY_HALF_RATIO;
    const float rampAngle = -std::atan(AeonLayout::RAMP_SLOPE);
    const glm::vec3 asphalt(0.19f, 0.21f, 0.23f);
    renderer_.drawOrientedBox(
        glm::vec3(AeonLayout::RAMP_MID_X_M,
                  midY - AeonLayout::RAMP_SLAB_THICKNESS_M * AeonLayout::GEOMETRY_HALF_RATIO,
                  AeonLayout::WORLD_ORIGIN_Z_M),
        glm::vec3(AeonLayout::RAMP_LENGTH_X_M, AeonLayout::RAMP_SLAB_THICKNESS_M,
                  AeonLayout::RAMP_TOTAL_WIDTH_Z_M), rampAngle, asphalt);

    renderer_.drawBox(
        glm::vec3(AeonLayout::RAMP_START_X_M - AeonLayout::RAMP_APRON_LENGTH_M * AeonLayout::GEOMETRY_HALF_RATIO,
                  upperLevelY - AeonLayout::RAMP_SLAB_THICKNESS_M,
                  AeonLayout::WORLD_ORIGIN_Z_M),
        glm::vec3(AeonLayout::RAMP_APRON_LENGTH_M, AeonLayout::RAMP_SLAB_THICKNESS_M,
                  AeonLayout::RAMP_TOTAL_WIDTH_Z_M + AeonLayout::RAMP_APRON_EXTRA_WIDTH_M),
        glm::vec3(0.25f, 0.27f, 0.29f));

    renderer_.drawOrientedBox(
        glm::vec3(AeonLayout::RAMP_MID_X_M,
                  midY + AeonLayout::RAMP_EDGE_WALL_HEIGHT_M * AeonLayout::GEOMETRY_HALF_RATIO,
                  AeonLayout::WORLD_ORIGIN_Z_M),
        glm::vec3(AeonLayout::RAMP_LENGTH_X_M, AeonLayout::RAMP_EDGE_WALL_HEIGHT_M,
                  AeonLayout::RAMP_EDGE_WALL_THICKNESS_M),
        rampAngle, glm::vec3(0.43f, 0.45f, 0.47f));
}

inline void BasementRamp::drawRetainingWalls(float upperLevelY, float lowerLevelY) const {
    const float midY = (upperLevelY + lowerLevelY) * AeonLayout::GEOMETRY_HALF_RATIO;
    const float wallHeight = AeonLayout::RAMP_RETAINING_WALL_HEIGHT_M;
    const float wallOffsetZ = AeonLayout::RAMP_TOTAL_WIDTH_Z_M * AeonLayout::GEOMETRY_HALF_RATIO +
                              AeonLayout::RAMP_RETAINING_WALL_THICKNESS_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float rampAngle = -std::atan(AeonLayout::RAMP_SLOPE);
    const glm::vec3 wallColor(0.60f, 0.61f, 0.60f);
    const glm::vec3 capColor(0.72f, 0.72f, 0.69f);

    for (const float side : {-1.0f, 1.0f}) {
        const float wallZ = side * wallOffsetZ;
        renderer_.drawOrientedBox(
            glm::vec3(AeonLayout::RAMP_MID_X_M, midY + wallHeight * AeonLayout::GEOMETRY_HALF_RATIO, wallZ),
            glm::vec3(AeonLayout::RAMP_LENGTH_X_M, wallHeight,
                      AeonLayout::RAMP_RETAINING_WALL_THICKNESS_M), rampAngle, wallColor);
        renderer_.drawOrientedBox(
            glm::vec3(AeonLayout::RAMP_MID_X_M,
                      midY + wallHeight + AeonLayout::RAMP_RETAINING_CAP_THICKNESS_M * AeonLayout::GEOMETRY_HALF_RATIO,
                      wallZ),
            glm::vec3(AeonLayout::RAMP_LENGTH_X_M,
                      AeonLayout::RAMP_RETAINING_CAP_THICKNESS_M,
                      AeonLayout::RAMP_RETAINING_WALL_THICKNESS_M +
                          AeonLayout::RAMP_RETAINING_CAP_OVERHANG_M), rampAngle, capColor);
    }
}

inline void BasementRamp::drawSafetyMarkings(float upperLevelY, float lowerLevelY) const {
    const float midY = (upperLevelY + lowerLevelY) * AeonLayout::GEOMETRY_HALF_RATIO;
    const float rampAngle = -std::atan(AeonLayout::RAMP_SLOPE);
    const glm::vec3 white(0.83f, 0.84f, 0.81f);
    const glm::vec3 yellow(0.95f, 0.69f, 0.08f);
    const glm::vec3 black(0.10f, 0.11f, 0.12f);
    const float markingY = midY + AeonLayout::MARKING_HEIGHT_M * AeonLayout::GEOMETRY_HALF_RATIO;

    for (int dash = 0; dash < AeonLayout::RAMP_LANE_DASH_COUNT; ++dash) {
        const float x = AeonLayout::RAMP_START_X_M + AeonLayout::RAMP_LANE_DASH_START_OFFSET_M +
                        static_cast<float>(dash) * AeonLayout::RAMP_LANE_DASH_SPACING_M;
        for (const float laneZ : {AeonLayout::ENTRY_LANE_CENTER_Z_M,
                                  AeonLayout::EXIT_LANE_CENTER_Z_M}) {
            renderer_.drawOrientedBox(
                glm::vec3(x, AeonLayout::rampYAtX(x, upperLevelY, lowerLevelY) +
                                  AeonLayout::MARKING_HEIGHT_M * AeonLayout::GEOMETRY_HALF_RATIO, laneZ),
                glm::vec3(AeonLayout::RAMP_LANE_DASH_LENGTH_M,
                          AeonLayout::MARKING_HEIGHT_M,
                          AeonLayout::RAMP_LANE_MARKING_WIDTH_M), rampAngle, white);
        }
    }

    for (const float laneZ : {AeonLayout::ENTRY_LANE_CENTER_Z_M,
                              AeonLayout::EXIT_LANE_CENTER_Z_M}) {
        const float edgeZ = laneZ +
            (laneZ < 0.0f ? AeonLayout::RAMP_EDGE_MARKING_OFFSET_M
                          : -AeonLayout::RAMP_EDGE_MARKING_OFFSET_M);
        renderer_.drawOrientedBox(
            glm::vec3(AeonLayout::RAMP_MID_X_M, markingY, edgeZ),
            glm::vec3(AeonLayout::RAMP_LENGTH_X_M, AeonLayout::MARKING_HEIGHT_M,
                      AeonLayout::RAMP_LANE_MARKING_WIDTH_M), rampAngle, yellow);
    }

    const int stripeCount = static_cast<int>(
        AeonLayout::RAMP_LENGTH_X_M / AeonLayout::RAMP_SAFETY_STRIPE_SPACING_M);
    const float stripeY = midY + AeonLayout::RAMP_EDGE_WALL_HEIGHT_M +
                          AeonLayout::RAMP_SAFETY_STRIPE_HEIGHT_M * AeonLayout::GEOMETRY_HALF_RATIO;
    for (const float side : {-1.0f, 1.0f}) {
        const float wallFaceZ = side * (AeonLayout::RAMP_TOTAL_WIDTH_Z_M * AeonLayout::GEOMETRY_HALF_RATIO -
                                        AeonLayout::RAMP_STRIPE_FACE_OFFSET_M);
        renderer_.drawOrientedBox(
            glm::vec3(AeonLayout::RAMP_MID_X_M, stripeY, wallFaceZ),
            glm::vec3(AeonLayout::RAMP_LENGTH_X_M, AeonLayout::RAMP_SAFETY_STRIPE_HEIGHT_M,
                      AeonLayout::RAMP_STRIPE_DEPTH_M), rampAngle, black);
    }

    for (int stripe = 0; stripe <= stripeCount; ++stripe) {
        const float x = AeonLayout::RAMP_START_X_M + AeonLayout::RAMP_STRIPE_START_OFFSET_M +
                        static_cast<float>(stripe) * AeonLayout::RAMP_SAFETY_STRIPE_SPACING_M;
        if (x > AeonLayout::RAMP_END_X_M - AeonLayout::RAMP_STRIPE_END_MARGIN_M) continue;
        const float y = AeonLayout::rampYAtX(x, upperLevelY, lowerLevelY) +
                        AeonLayout::RAMP_EDGE_WALL_HEIGHT_M +
                        AeonLayout::RAMP_SAFETY_STRIPE_HEIGHT_M * AeonLayout::GEOMETRY_HALF_RATIO;
        for (const float side : {-1.0f, 1.0f}) {
            const float wallFaceZ = side * (AeonLayout::RAMP_TOTAL_WIDTH_Z_M * AeonLayout::GEOMETRY_HALF_RATIO -
                                            AeonLayout::RAMP_STRIPE_FACE_OFFSET_M);
            const float stripeZ = wallFaceZ - side * AeonLayout::RAMP_STRIPE_DEPTH_M;
            const glm::mat4 stripeModel =
                glm::translate(glm::mat4(1.0f), glm::vec3(x, y, stripeZ)) *
                glm::rotate(glm::mat4(1.0f), rampAngle +
                            side * AeonLayout::RAMP_SAFETY_STRIPE_ANGLE_RAD,
                            glm::vec3(0.0f, 0.0f, 1.0f)) *
                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,
                    -AeonLayout::RAMP_SAFETY_STRIPE_HEIGHT_M * AeonLayout::GEOMETRY_HALF_RATIO, 0.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(AeonLayout::RAMP_SAFETY_STRIPE_WIDTH_M,
                    AeonLayout::RAMP_SAFETY_STRIPE_HEIGHT_M, AeonLayout::RAMP_STRIPE_DEPTH_M));
            renderer_.draw(renderer_.boxMesh(), stripeModel, yellow);
        }
    }
}

inline void BasementRamp::drawEntrancePortal(float upperLevelY) const {
    const float portalX = AeonLayout::RAMP_START_X_M + AeonLayout::ENTRANCE_PORTAL_START_OFFSET_M;
    const float clearWidth = AeonLayout::ENTRANCE_PORTAL_CLEAR_WIDTH_M;
    const float pierWidth = AeonLayout::ENTRANCE_PORTAL_PIER_WIDTH_M;
    const float clearHeight = AeonLayout::ENTRANCE_PORTAL_CLEAR_HEIGHT_M;
    const float pierZ = clearWidth * AeonLayout::RAMP_PORTAL_SIDE_PIER_CENTER_RATIO + pierWidth * AeonLayout::GEOMETRY_HALF_RATIO;
    const glm::vec3 concrete(0.69f, 0.70f, 0.69f);

    for (const float side : {-1.0f, 1.0f}) {
        renderer_.drawBox(
            glm::vec3(portalX, upperLevelY, side * pierZ),
            glm::vec3(AeonLayout::ENTRANCE_PORTAL_DEPTH_M, clearHeight, pierWidth), concrete);
    }
    renderer_.drawBox(
        glm::vec3(portalX, upperLevelY + clearHeight, AeonLayout::WORLD_ORIGIN_Z_M),
        glm::vec3(AeonLayout::ENTRANCE_PORTAL_DEPTH_M,
                  AeonLayout::ENTRANCE_PORTAL_HEADER_HEIGHT_M,
                  clearWidth + 2.0f * pierWidth), glm::vec3(0.53f, 0.55f, 0.57f));
    renderer_.drawBox(
        glm::vec3(portalX - AeonLayout::ENTRANCE_PORTAL_DEPTH_M * AeonLayout::GEOMETRY_HALF_RATIO -
                      AeonLayout::ENTRANCE_FASCIA_FACE_GAP_M,
                  upperLevelY + clearHeight + AeonLayout::RAMP_FASCIA_VERTICAL_OFFSET_M,
                  AeonLayout::WORLD_ORIGIN_Z_M),
        glm::vec3(AeonLayout::ENTRANCE_FASCIA_THICKNESS_M,
                  AeonLayout::ENTRANCE_FASCIA_HEIGHT_M,
                  AeonLayout::ENTRANCE_FASCIA_WIDTH_M), glm::vec3(0.73f, 0.08f, 0.43f));
}

inline void ParkingGate::render(float openTimer, float upperLevelY, float lowerLevelY) const {
    const bool isOpen = openTimer > 0.0f;
    const float gateX = AeonLayout::ENTRY_GATE_X_M;
    const float baseY = AeonLayout::rampYAtX(gateX, upperLevelY, lowerLevelY);
    const float postZ = laneCenterZ_ - AeonLayout::BARRIER_ARM_LEN_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float outsideSign = boothOnPositiveZ_ ? 1.0f : -1.0f;
    const float boothZ = laneCenterZ_ +
        outsideSign * (AeonLayout::RAMP_LANE_W_M * AeonLayout::GEOMETRY_HALF_RATIO -
                       AeonLayout::GATE_BOOTH_SIDE_MARGIN_M);
    const float boothX = gateX - AeonLayout::GATE_BOOTH_LONGITUDINAL_OFFSET_M;

    // Inductive-loop strips and a card-reader pedestal make the checkpoint readable.
    for (int loop = 0; loop < AeonLayout::GATE_SENSOR_STRIP_COUNT; ++loop) {
        const float x = gateX - AeonLayout::GATE_SENSOR_START_OFFSET_M +
                        static_cast<float>(loop) * AeonLayout::GATE_SENSOR_SPACING_M;
        renderer_.drawOrientedBox(
            glm::vec3(x, AeonLayout::rampYAtX(x, upperLevelY, lowerLevelY) + AeonLayout::GATE_SENSOR_HEIGHT_M, laneCenterZ_),
            glm::vec3(AeonLayout::GATE_SENSOR_LENGTH_M,
                      AeonLayout::GATE_SENSOR_HEIGHT_M,
                      AeonLayout::RAMP_LANE_W_M - AeonLayout::GATE_SENSOR_EDGE_MARGIN_M),
            -std::atan(AeonLayout::RAMP_SLOPE), glm::vec3(0.72f, 0.73f, 0.70f));
    }

    renderer_.drawBox(
        glm::vec3(gateX, baseY, postZ),
        glm::vec3(AeonLayout::BARRIER_POST_SIZE_M, AeonLayout::BARRIER_POST_HEIGHT_M,
                  AeonLayout::BARRIER_POST_SIZE_M),
        glm::vec3(0.31f, 0.33f, 0.35f));

    renderer_.drawBox(
        glm::vec3(boothX, baseY, boothZ),
        glm::vec3(AeonLayout::GATE_BOOTH_WIDTH_M, AeonLayout::GATE_BOOTH_HEIGHT_M,
                  AeonLayout::GATE_BOOTH_DEPTH_M),
        glm::vec3(0.34f, 0.37f, 0.39f));
    renderer_.drawBox(
        glm::vec3(boothX, baseY + AeonLayout::GATE_BOOTH_HEIGHT_M, boothZ),
        glm::vec3(AeonLayout::GATE_BOOTH_CAP_WIDTH_M,
                  AeonLayout::GATE_BOOTH_CAP_HEIGHT_M,
                  AeonLayout::GATE_BOOTH_CAP_DEPTH_M),
        glm::vec3(0.22f, 0.24f, 0.26f));

    const float facingSign = boothOnPositiveZ_ ? -1.0f : 1.0f;
    const float screenZ = boothZ + facingSign * (AeonLayout::GATE_BOOTH_DEPTH_M * AeonLayout::GEOMETRY_HALF_RATIO +
                                                  AeonLayout::GATE_READER_DEPTH_M * AeonLayout::GEOMETRY_HALF_RATIO);
    renderer_.drawBox(
        glm::vec3(boothX + AeonLayout::GATE_READER_X_OFFSET_M,
                  baseY + AeonLayout::GATE_READER_Y_OFFSET_M, screenZ),
        glm::vec3(AeonLayout::GATE_READER_WIDTH_M, AeonLayout::GATE_READER_HEIGHT_M,
                  AeonLayout::GATE_READER_DEPTH_M),
        glm::vec3(0.08f, 0.16f, 0.19f));
    renderer_.drawBox(
        glm::vec3(boothX + AeonLayout::GATE_READER_X_OFFSET_M,
                  baseY + AeonLayout::GATE_STATUS_Y_OFFSET_M, screenZ),
        glm::vec3(AeonLayout::GATE_STATUS_WIDTH_M,
                  AeonLayout::GATE_STATUS_HEIGHT_M,
                  AeonLayout::GATE_STATUS_DEPTH_M),
        isOpen ? glm::vec3(0.10f, 0.82f, 0.27f)
               : glm::vec3(0.88f, 0.10f, 0.06f));

    const float angle = isOpen ? -AeonLayout::PI * AeonLayout::GEOMETRY_HALF_RATIO : 0.0f;
    const glm::mat4 pivot =
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(gateX,
                                 baseY + AeonLayout::BARRIER_PIVOT_HEIGHT_M,
                                 postZ)) *
        glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 arm =
        pivot *
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(0.0f, 0.0f, AeonLayout::BARRIER_ARM_LEN_M * AeonLayout::GEOMETRY_HALF_RATIO)) *
        glm::scale(glm::mat4(1.0f),
                   glm::vec3(AeonLayout::BARRIER_ARM_THICKNESS_M,
                             AeonLayout::BARRIER_ARM_THICKNESS_M,
                             AeonLayout::BARRIER_ARM_LEN_M));
    renderer_.draw(renderer_.boxMesh(), arm, glm::vec3(0.91f, 0.91f, 0.87f));

    const float stripeLength = AeonLayout::BARRIER_ARM_LEN_M /
                               static_cast<float>(AeonLayout::BARRIER_ARM_STRIPE_COUNT);
    for (int stripe = 0; stripe < AeonLayout::BARRIER_ARM_STRIPE_COUNT; ++stripe) {
        const glm::mat4 stripeModel =
            pivot *
            glm::translate(glm::mat4(1.0f),
                           glm::vec3(0.0f, AeonLayout::RAMP_STRIPE_FACE_OFFSET_Y_M,
                                     (static_cast<float>(stripe) + AeonLayout::GEOMETRY_HALF_RATIO) * stripeLength)) *
            glm::scale(glm::mat4(1.0f),
                       glm::vec3(AeonLayout::BARRIER_ARM_THICKNESS_M * AeonLayout::RAMP_STRIPE_THICKNESS_SCALE,
                                 AeonLayout::BARRIER_ARM_THICKNESS_M * AeonLayout::RAMP_STRIPE_THICKNESS_SCALE,
                                 stripeLength * AeonLayout::RAMP_STRIPE_LENGTH_SCALE));
        const glm::vec3 stripeColor = (stripe % 2 == 0)
            ? glm::vec3(0.88f, 0.12f, 0.08f)
            : glm::vec3(0.96f, 0.91f, 0.72f);
        renderer_.draw(renderer_.boxMesh(), stripeModel, stripeColor);
    }
}

inline void SceneRenderer::render(const ParkingWorld& world, const ElevatorSystem& elevator, bool topView) {
    if (exteriorVisible_) {
        mallExterior_.render();
        // Show the full ground-to-B1 ramp in the exterior view so the garage
        // entrance is visible from outside, matching the original layout.
        ramp_.render(AeonLayout::GROUND_LEVEL_Y_M,
                     AeonLayout::FLOOR_HEIGHT_B1_M, true);
        // Show vehicles that are still on the ramp (not yet inside the building).
        for (const auto& vehicle : world.vehicles()) {
            if (vehicle.position().x > AeonLayout::RAMP_END_X_M) continue;
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), vehicle.position()) *
                                    glm::rotate(glm::mat4(1.0f), vehicle.yaw(),
                                                glm::vec3(0.0f, 1.0f, 0.0f));
            renderer_.drawCar(model, glm::vec3(0.72f, 0.74f, 0.77f));
        }
        if (world.manualMode() && world.playerPosition().x <= AeonLayout::RAMP_END_X_M) {
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), world.playerPosition()) *
                                    glm::rotate(glm::mat4(1.0f), world.playerYaw(),
                                                glm::vec3(0.0f, 1.0f, 0.0f));
            renderer_.drawCar(model, glm::vec3(0.12f, 0.38f, 0.84f));
        }
        return;
    }

    const float floorY = world.floorY();
    drawShell(floorY);
    drawRealisticColumns(world, floorY);

    drawLaneMarkings(floorY);
    for (const auto& slot : world.slots()) drawParkingSlot(slot, floorY);
    ramp_.render(world.upperFloorY(), floorY,
                 world.floorIndex() == AeonLayout::FLOOR_INDEX_B1);
    guardStation_.render(world.entryBarrierOpenTimer(), world.exitBarrierOpenTimer(), floorY);
    drawLightFixtures(floorY);
    drawMotorbikeArea(floorY);
    drawElevatorCore(floorY, elevator);
    if (!topView) drawBasementCeiling(floorY, world.slots());

    for (const auto& bike : world.motorbikes()) drawMotorbike(bike);

    for (const auto& vehicle : world.vehicles()) {
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), vehicle.position()) *
                                glm::rotate(glm::mat4(1.0f), vehicle.yaw(),
                                            glm::vec3(0.0f, 1.0f, 0.0f));
        renderer_.drawCar(model, glm::vec3(0.72f, 0.74f, 0.77f));
    }

    if (world.manualMode()) {
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), world.playerPosition()) *
                                glm::rotate(glm::mat4(1.0f), world.playerYaw(),
                                            glm::vec3(0.0f, 1.0f, 0.0f));
        renderer_.drawCar(model, glm::vec3(0.12f, 0.38f, 0.84f));
    }
}

inline void SceneRenderer::drawShell(float floorY) {
    const float ceilingY = floorY + AeonLayout::FLOOR_TO_CEILING_M;
    const glm::vec3 floorColor(0.72f, 0.74f, 0.76f);       // Epoxy xám sáng bóng như ảnh thật
    const glm::vec3 wallUpperColor(0.92f, 0.93f, 0.95f);   // Nửa trên tường sơn trắng kem sáng
    const glm::vec3 wallLowerColor(0.55f, 0.58f, 0.62f);   // Nửa dưới tường sơn xám ghi đậm như ảnh
    const glm::vec3 wallStripeColor(0.98f, 0.78f, 0.05f);  // Chỉ phân cách vàng nghệ
    const float splitH = 1.30f;                            // Độ cao phân cách 2 màu (1.30m)
    const float upperH = AeonLayout::FLOOR_TO_CEILING_M - splitH;

    // Sàn tầng hầm Epoxy xám sáng bóng
    renderer_.drawBox(
        glm::vec3(AeonLayout::WORLD_ORIGIN_X_M,
                  floorY - AeonLayout::FLOOR_SLAB_THICKNESS_M,
                  AeonLayout::WORLD_ORIGIN_Z_M),
        glm::vec3(AeonLayout::ZONE_LENGTH_X_M, AeonLayout::FLOOR_SLAB_THICKNESS_M,
                  AeonLayout::ZONE_WIDTH_Z_M), floorColor);

    // Tường bao phía Nam (2 tông màu)
    renderer_.drawBox(glm::vec3(AeonLayout::WORLD_ORIGIN_X_M, floorY, AeonLayout::ZONE_MIN_Z_M),
                      glm::vec3(AeonLayout::ZONE_LENGTH_X_M, splitH, AeonLayout::WALL_THICKNESS_M), wallLowerColor);
    renderer_.drawBox(glm::vec3(AeonLayout::WORLD_ORIGIN_X_M, floorY + splitH, AeonLayout::ZONE_MIN_Z_M),
                      glm::vec3(AeonLayout::ZONE_LENGTH_X_M, upperH, AeonLayout::WALL_THICKNESS_M), wallUpperColor);
    renderer_.drawBox(glm::vec3(AeonLayout::WORLD_ORIGIN_X_M, floorY + splitH - 0.02f, AeonLayout::ZONE_MIN_Z_M + 0.01f),
                      glm::vec3(AeonLayout::ZONE_LENGTH_X_M, 0.04f, 0.02f), wallStripeColor);

    // Tường bao phía Bắc (có cửa mở sang khu vực xe máy, 2 tông màu)
    const float bikeOpeningSegmentX =
        (AeonLayout::ZONE_LENGTH_X_M - AeonLayout::BIKE_ZONE_ACCESS_OPENING_WIDTH_X_M) *
        AeonLayout::GEOMETRY_HALF_RATIO;
    const float bikeOpeningLeftCenterX = AeonLayout::ZONE_MIN_X_M +
                                          bikeOpeningSegmentX * AeonLayout::GEOMETRY_HALF_RATIO;
    const float bikeOpeningRightCenterX = AeonLayout::ZONE_MAX_X_M -
                                           bikeOpeningSegmentX * AeonLayout::GEOMETRY_HALF_RATIO;
    for (const float segmentCenterX : {bikeOpeningLeftCenterX, bikeOpeningRightCenterX}) {
        renderer_.drawBox(glm::vec3(segmentCenterX, floorY, AeonLayout::ZONE_MAX_Z_M - AeonLayout::WALL_THICKNESS_M),
                          glm::vec3(bikeOpeningSegmentX, splitH, AeonLayout::WALL_THICKNESS_M), wallLowerColor);
        renderer_.drawBox(glm::vec3(segmentCenterX, floorY + splitH, AeonLayout::ZONE_MAX_Z_M - AeonLayout::WALL_THICKNESS_M),
                          glm::vec3(bikeOpeningSegmentX, upperH, AeonLayout::WALL_THICKNESS_M), wallUpperColor);
        renderer_.drawBox(glm::vec3(segmentCenterX, floorY + splitH - 0.02f, AeonLayout::ZONE_MAX_Z_M - AeonLayout::WALL_THICKNESS_M - 0.01f),
                          glm::vec3(bikeOpeningSegmentX, 0.04f, 0.02f), wallStripeColor);
    }

    // Tường bao phía Đông (có cửa mở vào sảnh thang máy, 2 tông màu)
    const float liftOpeningSegmentZ =
        (AeonLayout::ZONE_WIDTH_Z_M - AeonLayout::ELEVATOR_OPENING_WIDTH_Z_M) *
        AeonLayout::GEOMETRY_HALF_RATIO;
    const float liftOpeningLowerCenterZ = AeonLayout::ZONE_MIN_Z_M +
                                           liftOpeningSegmentZ * AeonLayout::GEOMETRY_HALF_RATIO;
    const float liftOpeningUpperCenterZ = AeonLayout::ZONE_MAX_Z_M -
                                           liftOpeningSegmentZ * AeonLayout::GEOMETRY_HALF_RATIO;
    for (const float segmentCenterZ : {liftOpeningLowerCenterZ, liftOpeningUpperCenterZ}) {
        renderer_.drawBox(glm::vec3(AeonLayout::ZONE_MAX_X_M - AeonLayout::WALL_THICKNESS_M, floorY, segmentCenterZ),
                          glm::vec3(AeonLayout::WALL_THICKNESS_M, splitH, liftOpeningSegmentZ), wallLowerColor);
        renderer_.drawBox(glm::vec3(AeonLayout::ZONE_MAX_X_M - AeonLayout::WALL_THICKNESS_M, floorY + splitH, segmentCenterZ),
                          glm::vec3(AeonLayout::WALL_THICKNESS_M, upperH, liftOpeningSegmentZ), wallUpperColor);
        renderer_.drawBox(glm::vec3(AeonLayout::ZONE_MAX_X_M - AeonLayout::WALL_THICKNESS_M - 0.01f, floorY + splitH - 0.02f, segmentCenterZ),
                          glm::vec3(0.02f, 0.04f, liftOpeningSegmentZ), wallStripeColor);
    }

    // Tường bao phía Tây (có cửa mở ra dốc xe, 2 tông màu)
    const float westWallSegmentLength =
        (AeonLayout::ZONE_WIDTH_Z_M - AeonLayout::RAMP_TOTAL_WIDTH_Z_M) * AeonLayout::GEOMETRY_HALF_RATIO;
    const float westLowerWallCenterZ = AeonLayout::ZONE_MIN_Z_M + westWallSegmentLength * AeonLayout::GEOMETRY_HALF_RATIO;
    const float westUpperWallCenterZ = AeonLayout::RAMP_TOTAL_WIDTH_Z_M * AeonLayout::GEOMETRY_HALF_RATIO +
                                       westWallSegmentLength * AeonLayout::GEOMETRY_HALF_RATIO;
    for (const float wCenterZ : {westLowerWallCenterZ, westUpperWallCenterZ}) {
        renderer_.drawBox(glm::vec3(AeonLayout::ZONE_MIN_X_M, floorY, wCenterZ),
                          glm::vec3(AeonLayout::WALL_THICKNESS_M, splitH, westWallSegmentLength), wallLowerColor);
        renderer_.drawBox(glm::vec3(AeonLayout::ZONE_MIN_X_M, floorY + splitH, wCenterZ),
                          glm::vec3(AeonLayout::WALL_THICKNESS_M, upperH, westWallSegmentLength), wallUpperColor);
        renderer_.drawBox(glm::vec3(AeonLayout::ZONE_MIN_X_M + 0.01f, floorY + splitH - 0.02f, wCenterZ),
                          glm::vec3(0.02f, 0.04f, westWallSegmentLength), wallStripeColor);
    }
    (void)ceilingY;
}

inline void SceneRenderer::drawRealisticColumns(const ParkingWorld& world, float floorY) {
    const float colSize = AeonLayout::COLUMN_SIZE_M; // 0.60m
    const glm::vec3 upperColor(0.92f, 0.93f, 0.94f);      // Thân trên cột màu trắng sáng
    const glm::vec3 baseRubberColor(0.12f, 0.12f, 0.14f); // Chân đế bảo vệ màu đen cao su
    const glm::vec3 hazardYellow(0.98f, 0.78f, 0.05f);    // Vàng phản quang rực rỡ
    const glm::vec3 hazardBlack(0.12f, 0.12f, 0.14f);     // Đen cảnh báo
    const glm::vec3 silverConduit(0.80f, 0.83f, 0.86f);   // Ống mạ kẽm màu bạc sáng
    const glm::vec3 clampColor(0.28f, 0.30f, 0.32f);      // Đai kẹp ống kim loại

    const auto& cols = world.columns();
    for (size_t i = 0; i < cols.size(); ++i) {
        const auto& col = cols[i];

        // 1. Nửa trên cột (từ 1.15m lên 3.0m, cao 1.85m) sơn trắng kem
        renderer_.drawBox(glm::vec3(col.x, floorY + 1.15f, col.z),
                          glm::vec3(colSize, 1.85f, colSize), upperColor);

        // 2. Chân đế cao su đen bảo vệ chân cột (từ 0.0m đến 0.08m)
        renderer_.drawBox(glm::vec3(col.x, floorY, col.z),
                          glm::vec3(colSize + 0.03f, 0.08f, colSize + 0.03f), baseRubberColor);

        // 3. Khối nền màu vàng phản quang từ 0.08m đến 1.15m
        renderer_.drawBox(glm::vec3(col.x, floorY + 0.08f, col.z),
                          glm::vec3(colSize + 0.015f, 1.07f, colSize + 0.015f), hazardYellow);

        // Viền nẹp ngang màu đen trên đỉnh phần phản quang (1.12m đến 1.15m)
        renderer_.drawBox(glm::vec3(col.x, floorY + 1.12f, col.z),
                          glm::vec3(colSize + 0.025f, 0.03f, colSize + 0.025f), baseRubberColor);

        // 4. HỌA TIẾT SỌC CHỮ V CHÚC XUỐNG (CHEVRON STRIPES) TRÊN CẢ 4 MẶT CỘT
        const float bandLen = 0.38f;       // Chiều dài nhánh chữ V
        const float bandThick = 0.085f;    // Bề rộng dải sọc đen
        const float bandDepth = 0.02f;     // Độ dày nhô ra nhẹ
        const float faceDist = colSize * 0.5f + 0.012f; // Vị trí sát mặt cột

        auto drawArm = [&](const glm::vec3& center, float angleDeg, int axis) {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), center);
            if (axis == 0) {
                m = glm::rotate(m, glm::radians(angleDeg), glm::vec3(0.0f, 0.0f, 1.0f));
                m = glm::scale(m, glm::vec3(bandLen, bandThick, bandDepth));
            } else {
                m = glm::rotate(m, glm::radians(angleDeg), glm::vec3(1.0f, 0.0f, 0.0f));
                m = glm::scale(m, glm::vec3(bandDepth, bandThick, bandLen));
            }
            m = m * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, 0.0f));
            renderer_.draw(renderer_.boxMesh(), m, hazardBlack);
        };

        // 4 dải chữ V đen ở các tầng độ cao khác nhau từ sàn lên
        const float chevronHeights[4] = { 0.20f, 0.44f, 0.68f, 0.92f };
        for (const float chY : chevronHeights) {
            const float y = floorY + chY;

            // Mặt Nam (hướng -Z): Chữ V chúc xuống \/ ở trung tâm X
            drawArm(glm::vec3(col.x - 0.15f, y + 0.09f, col.z - faceDist), -35.0f, 0);
            drawArm(glm::vec3(col.x + 0.15f, y + 0.09f, col.z - faceDist),  35.0f, 0);

            // Mặt Bắc (hướng +Z): Chữ V chúc xuống \/ ở trung tâm X
            drawArm(glm::vec3(col.x - 0.15f, y + 0.09f, col.z + faceDist), -35.0f, 0);
            drawArm(glm::vec3(col.x + 0.15f, y + 0.09f, col.z + faceDist),  35.0f, 0);

            // Mặt Tây (hướng -X): Chữ V chúc xuống \/ ở trung tâm Z
            drawArm(glm::vec3(col.x - faceDist, y + 0.09f, col.z - 0.15f),  35.0f, 1);
            drawArm(glm::vec3(col.x - faceDist, y + 0.09f, col.z + 0.15f), -35.0f, 1);

            // Mặt Đông (hướng +X): Chữ V chúc xuống \/ ở trung tâm Z
            drawArm(glm::vec3(col.x + faceDist, y + 0.09f, col.z - 0.15f),  35.0f, 1);
            drawArm(glm::vec3(col.x + faceDist, y + 0.09f, col.z + 0.15f), -35.0f, 1);
        }

        // 5. ỐNG KỸ THUẬT MẠ KẼM BẠC SÁNG KẸP DỌC THÂN CỘT (như trong ảnh thật)
        const float conduitX = col.x + (colSize * 0.5f + 0.07f);
        const float conduitZ = col.z + 0.06f;
        // Ống kim loại hình trụ 3D tròn chạy suốt từ sàn lên trần (cao 3.0m, bán kính 0.045m)
        renderer_.drawPipeY(glm::vec3(conduitX, floorY, conduitZ),
                            AeonLayout::FLOOR_TO_CEILING_M, 0.045f, silverConduit);
        // Các đai kẹp ôm ống bằng thép mạ kẽm cứ mỗi 0.65m
        for (float cy = 0.35f; cy < AeonLayout::FLOOR_TO_CEILING_M; cy += 0.65f) {
            renderer_.drawPipeY(glm::vec3(conduitX, floorY + cy, conduitZ),
                                0.035f, 0.056f, clampColor);
        }
    }
}

inline void SceneRenderer::drawBasementCeiling(float floorY, const std::vector<ParkingSlot>& slots) {
    if (!ceilingVisible_) return;

    const float ceilingY = floorY + AeonLayout::FLOOR_TO_CEILING_M;

    // 1. TẤM SÀN TRẦN CHÍNH (Main Concrete Ceiling Slab)
    const glm::vec3 ceilingColor(0.92f, 0.93f, 0.94f); // Trắng sáng bê tông phẳng
    renderer_.drawBox(
        glm::vec3(AeonLayout::WORLD_ORIGIN_X_M, ceilingY, AeonLayout::WORLD_ORIGIN_Z_M),
        glm::vec3(AeonLayout::ZONE_LENGTH_X_M, 0.25f, AeonLayout::ZONE_WIDTH_Z_M),
        ceilingColor);

    // Trần sảnh thang máy
    renderer_.drawBox(
        glm::vec3(AeonLayout::ELEVATOR_CORE_CENTER_X_M, ceilingY, AeonLayout::ELEVATOR_CORE_CENTER_Z_M),
        glm::vec3(AeonLayout::ELEVATOR_CORE_WIDTH_X_M, 0.25f, AeonLayout::ELEVATOR_CORE_DEPTH_Z_M),
        ceilingColor);

    // Trần bãi xe máy
    renderer_.drawBox(
        glm::vec3(AeonLayout::BIKE_ZONE_CENTER_X_M, ceilingY, AeonLayout::BIKE_ZONE_CENTER_Z_M),
        glm::vec3(AeonLayout::BIKE_ZONE_WIDTH_X_M, 0.25f, AeonLayout::BIKE_ZONE_DEPTH_Z_M),
        ceilingColor);

    // 2. MẠNG LƯỚI DẦM BÊ TÔNG CHỊU LỰC TO BẢN (Heavy Concrete Beams)
    // Dầm rộng 0.65m bằng chiều rộng cột, nhô xuống 0.42m tạo ô trần âm vuông vức
    const glm::vec3 beamColor(0.89f, 0.90f, 0.92f);
    const float beamH = 0.42f;
    const float beamW = 0.65f;
    const float beamBottomY = ceilingY - beamH;

    // Các dầm chạy dọc trục X nối liền các hàng cột
    const float beamZPositions[] = {
        -21.65f, -11.0f, -5.0f, 5.0f, 11.0f, 21.65f
    };
    for (const float bz : beamZPositions) {
        renderer_.drawBox(
            glm::vec3(AeonLayout::WORLD_ORIGIN_X_M, beamBottomY, bz),
            glm::vec3(AeonLayout::ZONE_LENGTH_X_M, beamH, beamW),
            beamColor);
    }

    // Các dầm chạy ngang trục Z nối các cột ngang
    for (int grid = 0; grid <= AeonLayout::COLUMN_GRID_X_COUNT; ++grid) {
        const float bx = AeonLayout::CORE_MIN_X_M + static_cast<float>(grid) * AeonLayout::COLUMN_SPACING_X_M;
        renderer_.drawBox(
            glm::vec3(bx, beamBottomY, AeonLayout::WORLD_ORIGIN_Z_M),
            glm::vec3(beamW, beamH, AeonLayout::ZONE_WIDTH_Z_M),
            beamColor);
    }
    // Dầm biên 2 đầu Đông & Tây
    renderer_.drawBox(glm::vec3(AeonLayout::ZONE_MIN_X_M + beamW * 0.5f, beamBottomY, AeonLayout::WORLD_ORIGIN_Z_M),
                      glm::vec3(beamW, beamH, AeonLayout::ZONE_WIDTH_Z_M), beamColor);
    renderer_.drawBox(glm::vec3(AeonLayout::ZONE_MAX_X_M - beamW * 0.5f, beamBottomY, AeonLayout::WORLD_ORIGIN_Z_M),
                      glm::vec3(beamW, beamH, AeonLayout::ZONE_WIDTH_Z_M), beamColor);

    // 3. ĐƯỜNG ỐNG THÔNG GIÓ BỌC BẠC SÁNG KIM LOẠI (Silver Insulated HVAC Ducts)
    // Ống gió to bản màu kim loại sáng có gân bích nối ống như trong ảnh thật
    const glm::vec3 silverDuctColor(0.82f, 0.85f, 0.88f);
    const glm::vec3 flangeColor(0.45f, 0.48f, 0.52f);
    for (const float ductZ : {-18.5f, 18.5f}) {
        // Thân ống gió to chạy dọc hầm
        renderer_.drawBox(
            glm::vec3(AeonLayout::WORLD_ORIGIN_X_M, ceilingY - 0.46f, ductZ),
            glm::vec3(AeonLayout::ZONE_LENGTH_X_M - 4.0f, 0.38f, 0.85f),
            silverDuctColor);

        // Các gờ bích nối ống kim loại cứ mỗi 1.8m
        for (float fx = AeonLayout::ZONE_MIN_X_M + 2.0f; fx < AeonLayout::ZONE_MAX_X_M - 2.0f; fx += 1.8f) {
            renderer_.drawBox(
                glm::vec3(fx, ceilingY - 0.47f, ductZ),
                glm::vec3(0.04f, 0.40f, 0.88f),
                flangeColor);
        }
    }

    // 4. ĐƯỜNG ỐNG CỨU HỎA PCCC MÀU ĐỎ HÌNH TRỤ 3D (3D Cylindrical Fire Sprinkler Pipes)
    const glm::vec3 fireRed(0.85f, 0.12f, 0.10f);
    const glm::vec3 flangeRed(0.68f, 0.08f, 0.06f);      // Mặt bích nối ống đỏ đậm
    const glm::vec3 unistrutColor(0.68f, 0.70f, 0.74f);  // Ty ren và cùm treo mạ kẽm
    const glm::vec3 sprinklerBrass(0.95f, 0.78f, 0.28f); // Đầu phun đồng thau

    // Hai đường ống chính PCCC D130 (bán kính 0.065m) chạy dọc 2 làn xe nép sát dầm
    for (const float aisleZ : AeonLayout::AISLE_CENTER_Z_M) {
        const float pipeZ = aisleZ - 0.80f;
        // Thân ống chính hình trụ tròn 3D chạy suốt chiều dài hầm
        renderer_.drawPipeX(
            glm::vec3(AeonLayout::WORLD_ORIGIN_X_M, ceilingY - 0.52f, pipeZ),
            AeonLayout::ZONE_LENGTH_X_M - 2.0f, 0.065f, fireRed);

        // Các khớp nối mặt bích tròn (flanges) nổi khối 3D cứ mỗi 6m
        for (float fx = AeonLayout::ZONE_MIN_X_M + 3.0f; fx < AeonLayout::ZONE_MAX_X_M - 2.0f; fx += 6.0f) {
            renderer_.drawPipeX(glm::vec3(fx, ceilingY - 0.52f, pipeZ), 0.05f, 0.082f, flangeRed);
        }

        // Hệ thống cùm treo và ty ren thép mạ kẽm kẹp ống vào đáy dầm
        for (int grid = 0; grid <= AeonLayout::COLUMN_GRID_X_COUNT; ++grid) {
            const float hx = AeonLayout::CORE_MIN_X_M + static_cast<float>(grid) * AeonLayout::COLUMN_SPACING_X_M;
            // Đai ôm tròn quanh ống
            renderer_.drawPipeX(glm::vec3(hx, ceilingY - 0.52f, pipeZ), 0.06f, 0.078f, unistrutColor);
            // 2 thanh ty ren tròn treo từ dầm trần xuống
            renderer_.drawPipeY(glm::vec3(hx, ceilingY - 0.52f, pipeZ - 0.09f),
                                0.52f, 0.012f, unistrutColor);
            renderer_.drawPipeY(glm::vec3(hx, ceilingY - 0.52f, pipeZ + 0.09f),
                                0.52f, 0.012f, unistrutColor);
            // Thanh giằng ngang Unistrut
            renderer_.drawBox(glm::vec3(hx, ceilingY - 0.52f - 0.075f, pipeZ),
                              glm::vec3(0.05f, 0.025f, 0.24f), unistrutColor);
        }
    }

    // Các ống nhánh PCCC D75 (bán kính 0.038m) chạy ngang qua các dãy ô đỗ xe cứ mỗi 4.2m
    for (int branch = 0; branch < 12; ++branch) {
        const float branchX = AeonLayout::ZONE_MIN_X_M + 3.0f + static_cast<float>(branch) * 4.2f;
        // Ống nhánh hình trụ 3D tròn
        renderer_.drawPipeZ(
            glm::vec3(branchX, ceilingY - 0.48f, AeonLayout::WORLD_ORIGIN_Z_M),
            AeonLayout::ZONE_WIDTH_Z_M - 6.0f, 0.038f, fireRed);

        // Khớp nối chữ T tại giao điểm ống chính và ống nhánh
        for (const float aisleZ : AeonLayout::AISLE_CENTER_Z_M) {
            const float pipeZ = aisleZ - 0.80f;
            renderer_.drawPipeY(glm::vec3(branchX, ceilingY - 0.52f, pipeZ),
                                0.08f, 0.072f, flangeRed);
        }

        // Đầu phun sprinkler nhỏ bằng đồng thau chúc xuống
        for (float sz : {-15.0f, -8.0f, 0.0f, 8.0f, 15.0f}) {
            // Cổ ren chúc xuống
            renderer_.drawPipeY(glm::vec3(branchX, ceilingY - 0.53f, sz),
                                0.05f, 0.014f, sprinklerBrass);
            // Vành tán nước xòe tròn (deflector disk)
            renderer_.drawPipeY(glm::vec3(branchX, ceilingY - 0.54f, sz),
                                0.01f, 0.035f, sprinklerBrass);
        }
    }

    // 5. MÁNG CÁP ĐIỆN KỸ THUẬT (Galvanized Cable Trays)
    const glm::vec3 cableTrayColor(0.70f, 0.73f, 0.76f);
    for (const float aisleZ : AeonLayout::AISLE_CENTER_Z_M) {
        const float trayZ = aisleZ + 0.80f;
        renderer_.drawBox(
            glm::vec3(AeonLayout::WORLD_ORIGIN_X_M, ceilingY - 0.48f, trayZ),
            glm::vec3(AeonLayout::ZONE_LENGTH_X_M - 2.0f, 0.08f, 0.35f),
            cableTrayColor);
    }

    // 6. CỤM ỐNG KỸ THUẬT UỐN LƯỢN ZÍC ZẮC TRÊN TƯỜNG (3D Round Wall Conduit)
    const glm::vec3 wallPipeSilver(0.78f, 0.82f, 0.85f);
    const float serpStartX = AeonLayout::ZONE_MAX_X_M - 3.5f;
    const float serpZ = AeonLayout::ZONE_MIN_Z_M + 0.08f;
    for (int loop = 0; loop < 4; ++loop) {
        const float py = floorY + 1.4f + static_cast<float>(loop) * 0.22f;
        renderer_.drawPipeX(glm::vec3(serpStartX, py, serpZ), 2.2f, 0.032f, wallPipeSilver);
    }
    // Các cút co vuông góc nối zíc zắc
    renderer_.drawPipeY(glm::vec3(serpStartX - 1.1f, floorY + 1.4f, serpZ), 0.22f, 0.032f, wallPipeSilver);
    renderer_.drawPipeY(glm::vec3(serpStartX + 1.1f, floorY + 1.62f, serpZ), 0.22f, 0.032f, wallPipeSilver);
    renderer_.drawPipeY(glm::vec3(serpStartX - 1.1f, floorY + 1.84f, serpZ), 0.22f, 0.032f, wallPipeSilver);

    // 7. CẢM BIẾN VÀ ĐÈN BÁO Ô ĐỖ XE THÔNG MINH (PGS LED Indicators)
    for (const auto& slot : slots) {
        const float indZ = slot.center().z - static_cast<float>(slot.rowSide()) * (AeonLayout::STALL_L_M * 0.5f + 0.25f);
        renderer_.drawBox(
            glm::vec3(slot.center().x, ceilingY - 0.42f, indZ),
            glm::vec3(0.14f, 0.10f, 0.14f),
            glm::vec3(0.92f, 0.92f, 0.92f));

        const glm::vec3 ledColor = (slot.state() == ParkingSlot::State::Free)
            ? glm::vec3(0.15f, 0.98f, 0.25f)
            : glm::vec3(0.98f, 0.15f, 0.15f);
        renderer_.drawBox(
            glm::vec3(slot.center().x, ceilingY - 0.46f, indZ),
            glm::vec3(0.08f, 0.04f, 0.08f),
            ledColor);
    }
}

inline void SceneRenderer::drawLaneMarkings(float floorY) {
    for (const float aisleZ : AeonLayout::AISLE_CENTER_Z_M) {
        for (int marker = 0; marker < AeonLayout::COLUMN_GRID_X_COUNT; ++marker) {
            const float x = AeonLayout::CORE_MIN_X_M +
                            (static_cast<float>(marker) + AeonLayout::GEOMETRY_HALF_RATIO) *
                                AeonLayout::COLUMN_SPACING_X_M;
            drawArrow(x, AeonLayout::inboundTrackZ(aisleZ), 0.0f, floorY);
            drawArrow(x, AeonLayout::outboundTrackZ(aisleZ), AeonLayout::PI, floorY);
        }
    }
    const float perimeterNorthZ = AeonLayout::ZONE_MAX_Z_M -
                                  AeonLayout::PERIMETER_LANE_W_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float perimeterSouthZ = AeonLayout::ZONE_MIN_Z_M +
                                  AeonLayout::PERIMETER_LANE_W_M * AeonLayout::GEOMETRY_HALF_RATIO;
    drawArrow(AeonLayout::WORLD_ORIGIN_X_M, perimeterNorthZ, AeonLayout::PI, floorY);
    drawArrow(AeonLayout::WORLD_ORIGIN_X_M, perimeterSouthZ, 0.0f, floorY);
    drawArrow(AeonLayout::ZONE_MAX_X_M -
                  AeonLayout::PERIMETER_LANE_W_M * AeonLayout::GEOMETRY_HALF_RATIO,
              AeonLayout::WORLD_ORIGIN_Z_M, AeonLayout::PI * AeonLayout::GEOMETRY_HALF_RATIO,
              floorY);
}

inline void SceneRenderer::drawArrow(float x, float z, float yaw, float floorY) {
    const glm::mat4 base =
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(x, floorY + AeonLayout::MARKING_HEIGHT_M, z)) *
        glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 arrowColor(0.92f, 0.88f, 0.66f);
    const glm::mat4 shaft =
        base * glm::translate(glm::mat4(1.0f),
                              glm::vec3(-AeonLayout::ARROW_LENGTH_M * AeonLayout::ARROW_SHAFT_OFFSET_RATIO,
                                        0.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f),
                   glm::vec3(AeonLayout::ARROW_LENGTH_M * AeonLayout::ARROW_SHAFT_LENGTH_RATIO,
                             AeonLayout::MARKING_HEIGHT_M, AeonLayout::ARROW_STROKE_M));
    renderer_.draw(renderer_.boxMesh(), shaft, arrowColor);
    for (int side : {-1, 1}) {
        const float angle = side > 0 ? AeonLayout::ARROW_HEAD_ANGLE_RAD
                                     : -AeonLayout::ARROW_HEAD_ANGLE_RAD;
        const glm::mat4 head =
            base * glm::translate(glm::mat4(1.0f),
                                  glm::vec3(AeonLayout::ARROW_LENGTH_M * AeonLayout::ARROW_HEAD_OFFSET_RATIO,
                                            0.0f,
                                            static_cast<float>(side) * AeonLayout::ARROW_LENGTH_M *
                                                AeonLayout::ARROW_HEAD_SIDE_RATIO)) *
            glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f),
                       glm::vec3(AeonLayout::ARROW_LENGTH_M * AeonLayout::ARROW_HEAD_LENGTH_RATIO,
                                 AeonLayout::MARKING_HEIGHT_M, AeonLayout::ARROW_STROKE_M));
        renderer_.draw(renderer_.boxMesh(), head, arrowColor);
    }
}

inline void SceneRenderer::drawParkingSlot(const ParkingSlot& slot, float floorY) {
    // Sàn ô đỗ xe màu xám Epoxy đồng bộ với sàn hầm
    glm::vec3 fillColor(0.69f, 0.72f, 0.74f);
    if (slot.state() == ParkingSlot::State::Reserved) fillColor = glm::vec3(0.62f, 0.44f, 0.18f);
    if (slot.state() == ParkingSlot::State::Occupied) fillColor = glm::vec3(0.58f, 0.22f, 0.20f);
    renderer_.drawBox(
        glm::vec3(slot.center().x, floorY + AeonLayout::BAY_FILL_FLOOR_OFFSET_M, slot.center().z),
        glm::vec3(AeonLayout::STALL_W_M - AeonLayout::BAY_MARKING_WIDTH_M * 2.0f,
                  AeonLayout::MARKING_HEIGHT_M * AeonLayout::BAY_FILL_HEIGHT_FACTOR,
                  AeonLayout::STALL_L_M - AeonLayout::BAY_MARKING_WIDTH_M * 2.0f), fillColor);

    // Vạch kẻ sơn ô đỗ xe màu VÀNG NGHỆ TƯƠI như ảnh thực tế
    const glm::vec3 marking(0.96f, 0.78f, 0.08f);
    const float leftX = slot.center().x - AeonLayout::STALL_W_M * AeonLayout::GEOMETRY_HALF_RATIO;
    const float rightX = slot.center().x + AeonLayout::STALL_W_M * AeonLayout::GEOMETRY_HALF_RATIO;
    renderer_.drawBox(glm::vec3(leftX, floorY + AeonLayout::MARKING_HEIGHT_M, slot.center().z),
        glm::vec3(AeonLayout::BAY_MARKING_WIDTH_M, AeonLayout::MARKING_HEIGHT_M,
                  AeonLayout::STALL_L_M), marking);
    renderer_.drawBox(glm::vec3(rightX, floorY + AeonLayout::MARKING_HEIGHT_M, slot.center().z),
        glm::vec3(AeonLayout::BAY_MARKING_WIDTH_M, AeonLayout::MARKING_HEIGHT_M,
                  AeonLayout::STALL_L_M), marking);
    const float rearZ = slot.center().z + static_cast<float>(slot.rowSide()) *
                        AeonLayout::STALL_L_M * AeonLayout::GEOMETRY_HALF_RATIO;
    renderer_.drawBox(glm::vec3(slot.center().x, floorY + AeonLayout::MARKING_HEIGHT_M, rearZ),
        glm::vec3(AeonLayout::STALL_W_M, AeonLayout::MARKING_HEIGHT_M,
                  AeonLayout::BAY_MARKING_WIDTH_M), marking);

    // THANH ỐNG THÉP CHẶN BÁNH XE MÀU VÀNG (3D Yellow Tubular Steel Stopper Rail) như trong ảnh
    const glm::vec3 pipeYellow(0.98f, 0.78f, 0.05f); // Vàng tươi rực rỡ sơn bóng
    const glm::vec3 boltColor(0.20f, 0.22f, 0.24f);  // Bu lông chân đế thép
    const float stopperZ = slot.center().z + static_cast<float>(slot.rowSide()) *
                           (AeonLayout::STALL_L_M * 0.5f - 0.60f);

    // 1. Thanh ống thép tròn nằm ngang màu vàng dài 2.1m, bán kính 0.045m (phi 90), cao cách sàn 0.10m
    renderer_.drawPipeX(glm::vec3(slot.center().x, floorY + 0.10f, stopperZ),
                        2.10f, 0.045f, pipeYellow);

    // 2. Hai chân trụ ống thép tròn đỡ thanh ngang hàn bắt vít xuống sàn (tại X = +-0.65m)
    for (float px : {-0.65f, 0.65f}) {
        renderer_.drawPipeY(glm::vec3(slot.center().x + px, floorY, stopperZ),
                            0.10f, 0.035f, pipeYellow);
        renderer_.drawBox(glm::vec3(slot.center().x + px, floorY, stopperZ),
                          glm::vec3(0.18f, 0.015f, 0.18f), boltColor);
    }
}

inline void SceneRenderer::drawLightFixtures(float floorY) {
    const float ceilingY = floorY + AeonLayout::FLOOR_TO_CEILING_M;
    const glm::vec3 battenWhite(0.92f, 0.93f, 0.95f); // Thân máng đèn trắng
    const glm::vec3 ledLight(1.0f, 0.99f, 0.95f);     // Ánh sáng LED trắng rực rỡ
    const glm::vec3 hangerColor(0.24f, 0.25f, 0.26f); // Dây ty treo đèn kim loại

    // 5 hàng đèn LED đôi dài chạy suốt tầng hầm
    const float lightZPositions[] = {
        -16.0f, -8.0f, 0.0f, 8.0f, 16.0f
    };

    for (const float z : lightZPositions) {
        for (int grid = 0; grid <= AeonLayout::COLUMN_GRID_X_COUNT + 1; ++grid) {
            const float x = AeonLayout::CORE_MIN_X_M + 1.25f +
                            static_cast<float>(grid) * (AeonLayout::COLUMN_SPACING_X_M * 0.85f);
            if (x > AeonLayout::ZONE_MAX_X_M - 2.0f) continue;

            // Dây ti treo thả từ trần
            renderer_.drawBox(glm::vec3(x - 0.85f, ceilingY - 0.44f, z),
                              glm::vec3(0.02f, 0.44f, 0.02f), hangerColor);
            renderer_.drawBox(glm::vec3(x + 0.85f, ceilingY - 0.44f, z),
                              glm::vec3(0.02f, 0.44f, 0.02f), hangerColor);

            // Máng đèn công nghiệp
            renderer_.drawBox(glm::vec3(x, ceilingY - 0.46f, z),
                              glm::vec3(2.2f, 0.07f, 0.22f), battenWhite);

            // Bóng đèn tuýp LED đôi phát sáng
            renderer_.drawBox(glm::vec3(x, ceilingY - 0.485f, z),
                              glm::vec3(2.0f, 0.035f, 0.16f), ledLight);
        }
    }
}

inline void SceneRenderer::drawMotorbikeArea(float floorY) {
    const glm::vec3 bayColor(0.13f, 0.19f, 0.16f);
    const glm::vec3 lineColor(0.86f, 0.87f, 0.80f);
    renderer_.drawBox(glm::vec3(AeonLayout::BIKE_ZONE_CENTER_X_M,
        floorY + AeonLayout::BAY_FILL_FLOOR_OFFSET_M, AeonLayout::BIKE_ZONE_CENTER_Z_M),
        glm::vec3(AeonLayout::BIKE_ZONE_WIDTH_X_M, AeonLayout::MARKING_HEIGHT_M,
                  AeonLayout::BIKE_ZONE_DEPTH_Z_M), bayColor);
    renderer_.drawBox(glm::vec3(AeonLayout::BIKE_ZONE_CENTER_X_M,
        floorY + AeonLayout::BIKE_ZONE_ACCESS_PATH_THICKNESS_M * AeonLayout::GEOMETRY_HALF_RATIO,
        AeonLayout::ZONE_MAX_Z_M + AeonLayout::BIKE_ZONE_GAP_FROM_ZONE_M *
                                     AeonLayout::GEOMETRY_HALF_RATIO),
        glm::vec3(AeonLayout::BIKE_ZONE_WIDTH_X_M, AeonLayout::BIKE_ZONE_ACCESS_PATH_THICKNESS_M,
                  AeonLayout::BIKE_ZONE_GAP_FROM_ZONE_M), bayColor);
    for (int row = 0; row < AeonLayout::BIKE_ROW_COUNT; ++row) {
        for (int index = 0; index <= AeonLayout::BIKE_STALLS_PER_ROW; ++index) {
            const float x = AeonLayout::motorbikeSlotX(0) - AeonLayout::BIKE_STALL_W_M * AeonLayout::GEOMETRY_HALF_RATIO +
                            static_cast<float>(index) * AeonLayout::BIKE_STALL_W_M;
            renderer_.drawBox(glm::vec3(x, floorY + AeonLayout::MARKING_HEIGHT_M,
                AeonLayout::motorbikeSlotZ(row)),
                glm::vec3(AeonLayout::BIKE_STALL_LINE_WIDTH_M, AeonLayout::MARKING_HEIGHT_M,
                          AeonLayout::BIKE_STALL_L_M), lineColor);
        }
    }
    renderer_.drawBox(glm::vec3(AeonLayout::BIKE_ZONE_CENTER_X_M,
        floorY + AeonLayout::MARKING_HEIGHT_M,
        AeonLayout::BIKE_ZONE_CENTER_Z_M),
        glm::vec3(AeonLayout::BIKE_ZONE_WIDTH_X_M, AeonLayout::MARKING_HEIGHT_M,
                  AeonLayout::BIKE_STALL_LINE_WIDTH_M), lineColor);
}

inline void SceneRenderer::drawMotorbike(const ParkedMotorbike& bike) {
    const glm::vec3 rubber(0.035f, 0.04f, 0.045f);
    const glm::vec3 frame(0.72f, 0.12f, 0.08f);
    const glm::vec3 seat(0.06f, 0.065f, 0.07f);
    const float y = bike.position.y;
    // Two simplified wheels are boxes because the common renderer exposes a unit box mesh.
    renderer_.drawBox(glm::vec3(bike.position.x, y + AeonLayout::BIKE_WHEEL_RADIUS_M,
        bike.position.z - AeonLayout::BIKE_STALL_L_M * AeonLayout::BIKE_WHEEL_OFFSET_RATIO),
        glm::vec3(AeonLayout::BIKE_WHEEL_WIDTH_M, AeonLayout::BIKE_WHEEL_RADIUS_M * 2.0f,
                  AeonLayout::BIKE_WHEEL_RADIUS_M * AeonLayout::BIKE_WHEEL_THICKNESS_RATIO), rubber);
    renderer_.drawBox(glm::vec3(bike.position.x, y + AeonLayout::BIKE_WHEEL_RADIUS_M,
        bike.position.z + AeonLayout::BIKE_STALL_L_M * AeonLayout::BIKE_WHEEL_OFFSET_RATIO),
        glm::vec3(AeonLayout::BIKE_WHEEL_WIDTH_M, AeonLayout::BIKE_WHEEL_RADIUS_M * 2.0f,
                  AeonLayout::BIKE_WHEEL_RADIUS_M * AeonLayout::BIKE_WHEEL_THICKNESS_RATIO), rubber);
    renderer_.drawBox(glm::vec3(bike.position.x, y + AeonLayout::BIKE_WHEEL_RADIUS_M * AeonLayout::BIKE_BODY_RAISE_RATIO,
        bike.position.z), glm::vec3(AeonLayout::BIKE_BODY_WIDTH_M,
        AeonLayout::BIKE_BODY_HEIGHT_M, AeonLayout::BIKE_BODY_LENGTH_M * AeonLayout::BIKE_BODY_LENGTH_RATIO), frame);
    renderer_.drawBox(glm::vec3(bike.position.x, y + AeonLayout::BIKE_WHEEL_RADIUS_M * AeonLayout::BIKE_SEAT_RAISE_RATIO,
        bike.position.z - AeonLayout::BIKE_SEAT_LENGTH_M * AeonLayout::BIKE_SEAT_Z_OFFSET_RATIO),
        glm::vec3(AeonLayout::BIKE_BODY_WIDTH_M * AeonLayout::BIKE_SEAT_WIDTH_RATIO, AeonLayout::BIKE_BODY_HEIGHT_M * AeonLayout::BIKE_SEAT_HEIGHT_RATIO,
                  AeonLayout::BIKE_SEAT_LENGTH_M), seat);
    renderer_.drawBox(glm::vec3(bike.position.x, y + AeonLayout::BIKE_WHEEL_RADIUS_M * AeonLayout::BIKE_HANDLEBAR_RAISE_RATIO,
        bike.position.z + AeonLayout::BIKE_BODY_LENGTH_M * AeonLayout::BIKE_HANDLEBAR_Z_RATIO),
        glm::vec3(AeonLayout::BIKE_HANDLEBAR_WIDTH_M, AeonLayout::BIKE_WHEEL_WIDTH_M,
                  AeonLayout::BIKE_WHEEL_WIDTH_M), glm::vec3(0.15f, 0.16f, 0.17f));
    renderer_.drawBox(glm::vec3(bike.position.x, y + AeonLayout::BIKE_WHEEL_RADIUS_M * 2.0f,
        bike.position.z + AeonLayout::BIKE_BODY_LENGTH_M * AeonLayout::BIKE_FRONT_LAMP_Z_RATIO),
        glm::vec3(AeonLayout::BIKE_BODY_WIDTH_M * AeonLayout::BIKE_FRONT_LAMP_WIDTH_RATIO,
                  AeonLayout::BIKE_BODY_HEIGHT_M * AeonLayout::BIKE_FRONT_LAMP_HEIGHT_RATIO,
                  AeonLayout::BIKE_BODY_LENGTH_M * AeonLayout::BIKE_FRONT_LAMP_LENGTH_RATIO), glm::vec3(0.88f, 0.84f, 0.68f));
}

inline void SceneRenderer::drawElevatorCore(float floorY, const ElevatorSystem& elevator) {
    const float leftX = AeonLayout::ELEVATOR_CORE_CENTER_X_M -
                        AeonLayout::ELEVATOR_CORE_WIDTH_X_M * AeonLayout::GEOMETRY_HALF_RATIO;
    // Lobby slab at the currently viewed floor.
    renderer_.drawBox(glm::vec3(AeonLayout::ELEVATOR_CORE_CENTER_X_M,
        floorY + AeonLayout::ELEVATOR_LOBBY_FLOOR_THICKNESS_M * AeonLayout::GEOMETRY_HALF_RATIO,
        AeonLayout::ELEVATOR_CORE_CENTER_Z_M),
        glm::vec3(AeonLayout::ELEVATOR_CORE_WIDTH_X_M,
                  AeonLayout::ELEVATOR_LOBBY_FLOOR_THICKNESS_M,
                  AeonLayout::ELEVATOR_CORE_DEPTH_Z_M), glm::vec3(0.38f, 0.40f, 0.42f));

    const float walkwayLength = AeonLayout::ELEVATOR_CORE_CENTER_X_M -
        AeonLayout::ELEVATOR_CORE_WIDTH_X_M * AeonLayout::GEOMETRY_HALF_RATIO -
        AeonLayout::ZONE_MAX_X_M;
    if (walkwayLength > 0.0f) {
        renderer_.drawBox(glm::vec3(AeonLayout::ZONE_MAX_X_M + walkwayLength *
            AeonLayout::GEOMETRY_HALF_RATIO, floorY + AeonLayout::ELEVATOR_WALKWAY_HEIGHT_M *
            AeonLayout::GEOMETRY_HALF_RATIO, AeonLayout::ELEVATOR_CORE_CENTER_Z_M),
            glm::vec3(walkwayLength, AeonLayout::ELEVATOR_WALKWAY_HEIGHT_M,
                      AeonLayout::ELEVATOR_WALKWAY_WIDTH_M), glm::vec3(0.42f, 0.44f, 0.46f));
    }

    // Two illustrative lift shafts. Exact AEON location/size are not surveyed.
    const float shaftCenterX = leftX + AeonLayout::ELEVATOR_SHAFT_DEPTH_X_M *
        AeonLayout::GEOMETRY_HALF_RATIO + AeonLayout::ELEVATOR_DOOR_THICKNESS_M;
    const float doorX = leftX + AeonLayout::ELEVATOR_DOOR_THICKNESS_M *
        AeonLayout::GEOMETRY_HALF_RATIO;

    // Draw cabin only when it is near the currently viewed floor (within ~1 storey).
    const float cabinY = elevator.cabinY();
    const bool cabinVisibleHere =
        std::abs(cabinY - floorY) < (AeonLayout::FLOOR_TO_CEILING_M + 0.5f);

    for (int shaft = 0; shaft < AeonLayout::ELEVATOR_SHAFT_COUNT; ++shaft) {
        const float shaftZ = AeonLayout::ELEVATOR_CORE_CENTER_Z_M +
            (static_cast<float>(shaft) -
             static_cast<float>(AeonLayout::ELEVATOR_SHAFT_COUNT - 1) *
                 AeonLayout::GEOMETRY_HALF_RATIO) * AeonLayout::ELEVATOR_SHAFT_CENTER_SPACING_Z_M;

        // Shaft walls (dark interior)
        renderer_.drawBox(glm::vec3(shaftCenterX,
            floorY + AeonLayout::FLOOR_TO_CEILING_M * AeonLayout::GEOMETRY_HALF_RATIO, shaftZ),
            glm::vec3(AeonLayout::ELEVATOR_SHAFT_DEPTH_X_M,
                      AeonLayout::FLOOR_TO_CEILING_M - AeonLayout::ELEVATOR_LOBBY_FLOOR_THICKNESS_M,
                      AeonLayout::ELEVATOR_SHAFT_WIDTH_Z_M), glm::vec3(0.22f, 0.24f, 0.27f));

        // Moving cabin (shared visual for both shafts for simplicity)
        if (cabinVisibleHere) {
            const float cabinCenterY = cabinY + AeonLayout::ELEVATOR_CABIN_HEIGHT_M *
                AeonLayout::GEOMETRY_HALF_RATIO;
            // Cabin body
            renderer_.drawBox(glm::vec3(shaftCenterX, cabinCenterY, shaftZ),
                glm::vec3(AeonLayout::ELEVATOR_CABIN_DEPTH_X_M,
                          AeonLayout::ELEVATOR_CABIN_HEIGHT_M,
                          AeonLayout::ELEVATOR_CABIN_WIDTH_Z_M),
                glm::vec3(0.78f, 0.80f, 0.84f));
            // Cabin floor accent
            renderer_.drawBox(glm::vec3(shaftCenterX,
                cabinY + AeonLayout::ELEVATOR_CABIN_FLOOR_THICKNESS_M * AeonLayout::GEOMETRY_HALF_RATIO,
                shaftZ),
                glm::vec3(AeonLayout::ELEVATOR_CABIN_DEPTH_X_M * 0.95f,
                          AeonLayout::ELEVATOR_CABIN_FLOOR_THICKNESS_M,
                          AeonLayout::ELEVATOR_CABIN_WIDTH_Z_M * 0.95f),
                glm::vec3(0.55f, 0.57f, 0.60f));
        }

        // Animated doors: two panels that slide apart (openAmount 0..1)
        const float open = elevator.doorOpenAmount();
        const float halfDoor = AeonLayout::ELEVATOR_DOOR_WIDTH_M * 0.5f;
        const float slide = open * (halfDoor + 0.05f);
        // Left door panel
        renderer_.drawBox(glm::vec3(doorX,
            floorY + AeonLayout::ELEVATOR_DOOR_HEIGHT_M * AeonLayout::GEOMETRY_HALF_RATIO,
            shaftZ - halfDoor * 0.5f - slide),
            glm::vec3(AeonLayout::ELEVATOR_DOOR_THICKNESS_M,
                      AeonLayout::ELEVATOR_DOOR_HEIGHT_M, halfDoor),
            glm::vec3(0.58f, 0.63f, 0.67f));
        // Right door panel
        renderer_.drawBox(glm::vec3(doorX,
            floorY + AeonLayout::ELEVATOR_DOOR_HEIGHT_M * AeonLayout::GEOMETRY_HALF_RATIO,
            shaftZ + halfDoor * 0.5f + slide),
            glm::vec3(AeonLayout::ELEVATOR_DOOR_THICKNESS_M,
                      AeonLayout::ELEVATOR_DOOR_HEIGHT_M, halfDoor),
            glm::vec3(0.58f, 0.63f, 0.67f));

        // Floor indicator sign above the door
        renderer_.drawBox(glm::vec3(doorX - AeonLayout::ELEVATOR_DOOR_THICKNESS_M *
            AeonLayout::GEOMETRY_HALF_RATIO,
            floorY + AeonLayout::ELEVATOR_DOOR_HEIGHT_M +
                AeonLayout::ELEVATOR_SIGN_HEIGHT_M * AeonLayout::GEOMETRY_HALF_RATIO,
            shaftZ),
            glm::vec3(AeonLayout::ELEVATOR_DOOR_THICKNESS_M,
                      AeonLayout::ELEVATOR_SIGN_HEIGHT_M, AeonLayout::ELEVATOR_SIGN_WIDTH_M),
            glm::vec3(0.12f, 0.40f, 0.53f));
    }
}

// -------------------------------------------------------------------------- HUD

class Hud {
public:
    explicit Hud(Renderer& renderer) : renderer_(renderer) {}

    void render(const ParkingWorld& world, bool exteriorVisible, int width, int height,
                const ElevatorSystem& elevator, const Camera& camera);

private:
    static std::array<const char*, AeonLayout::BITMAP_GLYPH_HEIGHT_CELLS> glyphRows(char character);
    void drawRectangle(float x, float y, float width, float height, const glm::vec3& color);
    void drawText(const std::string& value, float x, float y, float pixelSize,
                  const glm::vec3& color);

    Renderer& renderer_;
};

inline void Hud::render(const ParkingWorld& world, bool exteriorVisible, int width, int height,
                        const ElevatorSystem& elevator, const Camera& camera) {
    int parkedCount = 0;
    int longestParkedSeconds = 0;
    for (const auto& vehicle : world.vehicles()) {
        if (vehicle.phase() == Vehicle::Phase::Parked) {
            ++parkedCount;
            longestParkedSeconds = std::max(longestParkedSeconds,
                                            static_cast<int>(vehicle.parkedSeconds()));
        }
    }

    renderer_.setViewProjection(glm::ortho(0.0f, static_cast<float>(width), 0.0f,
                                           static_cast<float>(height), AeonLayout::HUD_ORTHO_NEAR_CLIP,
                                           AeonLayout::HUD_ORTHO_FAR_CLIP));

    const float panelX = AeonLayout::HUD_MARGIN_PX;
    const float panelWidth = exteriorVisible ? AeonLayout::HUD_EXTERIOR_WIDTH_PX : AeonLayout::HUD_WIDTH_PX;
    const float panelHeight = exteriorVisible ? AeonLayout::HUD_EXTERIOR_HEIGHT_PX : AeonLayout::HUD_PANEL_HEIGHT_WITH_EXTENSIONS_PX;
    const float panelY =
        static_cast<float>(height) - panelHeight - AeonLayout::HUD_MARGIN_PX;
    drawRectangle(panelX, panelY, panelWidth, panelHeight,
                  glm::vec3(0.045f, 0.065f, 0.085f));
    drawRectangle(panelX, panelY + panelHeight - AeonLayout::HUD_BAR_HEIGHT_PX,
                  panelWidth, AeonLayout::HUD_BAR_HEIGHT_PX,
                  glm::vec3(0.10f, 0.34f, 0.48f));

    const glm::vec3 white(0.92f, 0.95f, 0.97f);
    const glm::vec3 green(0.32f, 0.88f, 0.52f);
    const glm::vec3 cyan(0.30f, 0.85f, 0.95f);
    const glm::vec3 amber(0.98f, 0.72f, 0.28f);
    const glm::vec3 red(0.96f, 0.40f, 0.34f);
    const float textSize = AeonLayout::HUD_TEXT_PIXEL_SIZE;
    const float lineStep = AeonLayout::HUD_LINE_STEP_PX;
    const auto line = [&](const std::string& text, int index, const glm::vec3& color) {
        drawText(text, panelX + AeonLayout::HUD_TEXT_OFFSET_X_PX,
                 panelY + panelHeight - AeonLayout::HUD_PANEL_LINE_BOTTOM_PADDING_PX -
                     static_cast<float>(index) * lineStep,
                 textSize, color);
    };

    if (exteriorVisible) {
        line("AEON MALL DA NANG", 0, white);
        line("TTC PLAZA - THANH KHE", 1, white);
        line("4 PODIUM + 14 TOWER FLOORS", 2, white);
        line("EXTERIOR MASSING - ASSUMED", 3, amber);
        line("V TOGGLE EXTERIOR / B1", 4, white);
        return;
    }

    const std::string floorLabel = world.floorIndex() == AeonLayout::FLOOR_INDEX_B2
        ? "B2 LEVEL -7.0M ASSUMED" : "B1 LEVEL -3.5M ASSUMED";
    line(world.floorIndex() == AeonLayout::FLOOR_INDEX_B2
             ? "AEON PARKING - B2" : "AEON PARKING - B1", 0, white);
    line("ZONE A SAMPLE 64 / APPROX 430 TOTAL", 1, white);
    line("CARS " + std::to_string(world.carCount() + (world.manualMode() ? 1 : 0)), 2, white);
    line("CAR FREE " + std::to_string(world.freeSlotCount()), 3, green);
    line("CAR RESERVED " + std::to_string(world.reservedSlotCount()), 4, amber);
    line("CAR OCCUPIED " + std::to_string(world.occupiedSlotCount()), 5, red);
    line("MOTORBIKES " + std::to_string(world.motorbikeCount()) + "/" +
             std::to_string(AeonLayout::TOTAL_ZONE_A_BIKE_SLOTS) + " FREE " +
             std::to_string(world.freeMotorbikeSlotCount()), 6, white);
    line("PARKED " + std::to_string(parkedCount) + "  TIME " +
             std::to_string(longestParkedSeconds) + "S", 7, white);
    line(world.manualMode() ? "MODE AUTO PLUS MANUAL" : "MODE AUTO", 8, white);
    line("PAYMENT QUEUE " + std::to_string(world.paymentQueueCount()), 9, white);
    line(world.freeSlotCount() == 0 ? "CAR STATUS FULL" :
             (world.autoSpawn() ? "SPAWN ON" : "SPAWN OFF"),
         10, world.freeSlotCount() == 0 ? red : white);
    {
        const char* floorNames[] = {"G", "B1", "B2"};
        std::string liftLine = "LIFT ";
        liftLine += floorNames[elevator.currentFloor()];
        liftLine += " ";
        liftLine += elevator.phaseName();
        if (elevator.phase() == ElevatorSystem::Phase::Moving) {
            liftLine += " -> ";
            liftLine += floorNames[elevator.targetFloor()];
        }
        line(liftLine, 11, white);
    }
    line(floorLabel, 12, white);

    // Realistic Camera status & control hints
    line(std::string("VIEW: ") + camera.currentModeName(), 13, cyan);
    line(std::string("CTRL: ") + camera.currentModeHint(), 14, amber);
    line("C: GOC NHIN  U: TRAN HAM  K/3: CCTV  T: MAP 2D", 15, green);
    line("1 B1  2 B2  V NGOAI THAT  B/X XE MAY  M LAI XE", 16, white);
}

inline void Hud::drawRectangle(float x, float y, float width, float height,
                               const glm::vec3& color) {
    renderer_.drawBox(glm::vec3(x + width * AeonLayout::GEOMETRY_HALF_RATIO, y, 0.0f),
                      glm::vec3(width, height, AeonLayout::HUD_RECT_DEPTH_M), color);
}

inline void Hud::drawText(const std::string& value, float x, float y, float pixelSize,
                          const glm::vec3& color) {
    float cursorX = x;
    for (const char character : value) {
        const auto rows = glyphRows(character);
        for (int row = 0; row < AeonLayout::BITMAP_GLYPH_HEIGHT_CELLS; ++row) {
            for (int column = 0; column < AeonLayout::BITMAP_GLYPH_WIDTH_CELLS; ++column) {
                if (rows[static_cast<std::size_t>(row)][column] != '1') continue;
                const float pixelX = cursorX + static_cast<float>(column) * pixelSize;
                const float pixelY = y + static_cast<float>(AeonLayout::BITMAP_GLYPH_LAST_ROW_INDEX - row) * pixelSize;
                drawRectangle(pixelX, pixelY, pixelSize * AeonLayout::HUD_GLYPH_PIXEL_FILL_RATIO,
                                pixelSize * AeonLayout::HUD_GLYPH_PIXEL_FILL_RATIO, color);
            }
        }
        cursorX += pixelSize * static_cast<float>(AeonLayout::BITMAP_GLYPH_ADVANCE_CELLS);
    }
}

// Self-contained 5x7 bitmap font for the in-scene HUD.
inline std::array<const char*, AeonLayout::BITMAP_GLYPH_HEIGHT_CELLS> Hud::glyphRows(char character) {
    switch (character) {
    case 'A': return {"01110", "10001", "10001", "11111", "10001", "10001", "10001"};
    case 'B': return {"11110", "10001", "10001", "11110", "10001", "10001", "11110"};
    case 'C': return {"01111", "10000", "10000", "10000", "10000", "10000", "01111"};
    case 'D': return {"11110", "10001", "10001", "10001", "10001", "10001", "11110"};
    case 'E': return {"11111", "10000", "10000", "11110", "10000", "10000", "11111"};
    case 'F': return {"11111", "10000", "10000", "11110", "10000", "10000", "10000"};
    case 'G': return {"01111", "10000", "10000", "10111", "10001", "10001", "01111"};
    case 'H': return {"10001", "10001", "10001", "11111", "10001", "10001", "10001"};
    case 'I': return {"11111", "00100", "00100", "00100", "00100", "00100", "11111"};
    case 'J': return {"00111", "00010", "00010", "00010", "10010", "10010", "01100"};
    case 'K': return {"10001", "10010", "10100", "11000", "10100", "10010", "10001"};
    case 'L': return {"10000", "10000", "10000", "10000", "10000", "10000", "11111"};
    case 'M': return {"10001", "11011", "10101", "10101", "10001", "10001", "10001"};
    case 'N': return {"10001", "11001", "10101", "10011", "10001", "10001", "10001"};
    case 'O': return {"01110", "10001", "10001", "10001", "10001", "10001", "01110"};
    case 'P': return {"11110", "10001", "10001", "11110", "10000", "10000", "10000"};
    case 'Q': return {"01110", "10001", "10001", "10001", "10101", "10010", "01101"};
    case 'R': return {"11110", "10001", "10001", "11110", "10100", "10010", "10001"};
    case 'S': return {"01111", "10000", "10000", "01110", "00001", "00001", "11110"};
    case 'T': return {"11111", "00100", "00100", "00100", "00100", "00100", "00100"};
    case 'U': return {"10001", "10001", "10001", "10001", "10001", "10001", "01110"};
    case 'V': return {"10001", "10001", "10001", "10001", "10001", "01010", "00100"};
    case 'W': return {"10001", "10001", "10001", "10101", "10101", "10101", "01010"};
    case 'X': return {"10001", "10001", "01010", "00100", "01010", "10001", "10001"};
    case 'Y': return {"10001", "10001", "01010", "00100", "00100", "00100", "00100"};
    case 'Z': return {"11111", "00001", "00010", "00100", "01000", "10000", "11111"};
    case '0': return {"01110", "10001", "10011", "10101", "11001", "10001", "01110"};
    case '1': return {"00100", "01100", "00100", "00100", "00100", "00100", "01110"};
    case '2': return {"01110", "10001", "00001", "00010", "00100", "01000", "11111"};
    case '3': return {"11110", "00001", "00001", "01110", "00001", "00001", "11110"};
    case '4': return {"00010", "00110", "01010", "10010", "11111", "00010", "00010"};
    case '5': return {"11111", "10000", "10000", "11110", "00001", "00001", "11110"};
    case '6': return {"01110", "10000", "10000", "11110", "10001", "10001", "01110"};
    case '7': return {"11111", "00001", "00010", "00100", "01000", "01000", "01000"};
    case '8': return {"01110", "10001", "10001", "01110", "10001", "10001", "01110"};
    case '9': return {"01110", "10001", "10001", "01111", "00001", "00001", "01110"};
    case '-': return {"00000", "00000", "00000", "11111", "00000", "00000", "00000"};
    case '.': return {"00000", "00000", "00000", "00000", "00000", "00110", "00110"};
    case '/': return {"00001", "00010", "00010", "00100", "01000", "01000", "10000"};
    default:  return {"00000", "00000", "00000", "00000", "00000", "00000", "00000"};
    }
}
