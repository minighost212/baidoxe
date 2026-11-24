// ============================================================================
//  guard_station.cpp - Security checkpoint & barrier control booth at ramp foot.
// ============================================================================

#include "guard_station.h"
#include "aeon_layout.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

GuardStation::GuardStation(Renderer& renderer)
    : renderer_(renderer),
      center_(glm::vec3(AeonLayout::ENTRY_GATE_X_M - 1.30f, 0.0f, 0.0f)),
      entryBarrierX_(AeonLayout::ENTRY_GATE_X_M),
      exitBarrierX_(AeonLayout::ENTRY_GATE_X_M)
{
}

void GuardStation::render(float entryBarrierTimer, float exitBarrierTimer, float floorY) const {
    // 1. Raised concrete traffic island with hazard striping
    drawRaisedIsland(floorY);

    // 2. Central security guard workstation: desk, computer case, peripherals
    drawGuardDesk(floorY);

    // 3. Dual LCD surveillance monitors
    drawDualMonitors(floorY);

    // 4. Ergonomic guard chair
    drawGuardChair(floorY);

    // 5. High-visibility automatic barrier gate housings & boom arms
    // Entry barrier gate (controls incoming lane from the ramp)
    drawBarrierGate(glm::vec3(entryBarrierX_, floorY + islandHeight_, -0.32f),
                    3.20f, entryBarrierTimer, true);

    // Exit barrier gate (controls outgoing lane climbing the ramp)
    drawBarrierGate(glm::vec3(exitBarrierX_, floorY + islandHeight_, 0.32f),
                    3.20f, exitBarrierTimer, false);

    // 6. Heavy-duty rubber speed bumps on road surface
    drawSpeedBumps(floorY);

    // 7. Overhead License Plate Recognition (LPR) cameras suspended from ceiling
    drawOverheadCameras(floorY);

    // 8. Pavement guidance markings & direction arrows
    drawLaneDirectionArrows(floorY);
}

void GuardStation::drawRaisedIsland(float floorY) const {
    const float islandY = floorY;
    const float halfLen = islandLength_ * 0.5f;
    const float halfWid = islandWidth_ * 0.5f;

    // Dark grey concrete main slab
    const glm::vec3 concreteDark(0.24f, 0.26f, 0.28f);
    renderer_.drawBox(center_ + glm::vec3(0.0f, islandY, 0.0f),
                      glm::vec3(islandLength_ - 0.40f, islandHeight_, islandWidth_ - 0.16f),
                      concreteDark);

    // Rounded ends using 3D cylinders
    const glm::vec3 curbYellow(0.96f, 0.78f, 0.05f);
    const glm::vec3 curbBlack(0.12f, 0.13f, 0.14f);

    renderer_.drawPipeY(glm::vec3(center_.x - halfLen + 0.20f, islandY, center_.z),
                        islandHeight_, halfWid - 0.08f, concreteDark);
    renderer_.drawPipeY(glm::vec3(center_.x + halfLen - 0.20f, islandY, center_.z),
                        islandHeight_, halfWid - 0.08f, concreteDark);

    // Alternating Yellow & Black hazard curb perimeter (as seen in the reference photos)
    const float segLen = 0.32f;
    const int numSegs = static_cast<int>(islandLength_ / segLen);

    for (int i = 0; i < numSegs; ++i) {
        const float segX = center_.x - halfLen + (static_cast<float>(i) + 0.5f) * segLen;
        const glm::vec3 color = (i % 2 == 0) ? curbYellow : curbBlack;

        // North edge curb
        renderer_.drawBox(glm::vec3(segX, islandY, center_.z + halfWid - 0.05f),
                          glm::vec3(segLen, islandHeight_, 0.10f), color);

        // South edge curb
        renderer_.drawBox(glm::vec3(segX, islandY, center_.z - halfWid + 0.05f),
                          glm::vec3(segLen, islandHeight_, 0.10f), color);
    }

    // Curved hazard caps at both ends
    renderer_.drawBox(glm::vec3(center_.x - halfLen + 0.05f, islandY, center_.z),
                      glm::vec3(0.10f, islandHeight_, islandWidth_), curbYellow);
    renderer_.drawBox(glm::vec3(center_.x + halfLen - 0.05f, islandY, center_.z),
                      glm::vec3(0.10f, islandHeight_, islandWidth_), curbBlack);
}

void GuardStation::drawGuardDesk(float floorY) const {
    // Bảo vệ ngồi phía +X so với bàn, nhìn về -X (nhìn ra phía xe vào từ dốc).
    // => trái/phải của bàn chạy theo trục Z; phía trước bàn hướng -X.
    const float deskBaseY = floorY + islandHeight_;
    const glm::vec3 deskPos(center_.x, deskBaseY, center_.z);

    const glm::vec3 tableTopColor(0.85f, 0.88f, 0.90f);   // Mặt bàn xám sáng
    const glm::vec3 tableFrameColor(0.32f, 0.35f, 0.38f);  // Khung thép xám
    const glm::vec3 caseBlack(0.10f, 0.11f, 0.12f);        // Thùng máy tính

    // Mặt bàn: 0.70m (theo X, chiều sâu từ trước ra sau) × 0.60m (theo Z, chiều ngang)
    renderer_.drawBox(deskPos + glm::vec3(0.0f, 0.72f, 0.0f),
                      glm::vec3(0.70f, 0.04f, 1.10f), tableTopColor);

    // Chân đỡ trái (Z âm)
    renderer_.drawBox(deskPos + glm::vec3(0.0f, 0.0f, -0.50f),
                      glm::vec3(0.60f, 0.72f, 0.06f), tableFrameColor);

    // Tủ ngăn kéo phải (Z dương)
    renderer_.drawBox(deskPos + glm::vec3(0.0f, 0.0f, 0.36f),
                      glm::vec3(0.60f, 0.72f, 0.32f), tableFrameColor);

    // Tay nắm ngăn kéo
    for (float dy : {0.20f, 0.40f, 0.58f}) {
        renderer_.drawBox(deskPos + glm::vec3(-0.31f, dy, 0.36f),
                          glm::vec3(0.02f, 0.02f, 0.14f), glm::vec3(0.80f, 0.82f, 0.85f));
    }

    // Thùng máy tính đặt dưới bàn phía trước (phía -X)
    renderer_.drawBox(deskPos + glm::vec3(-0.28f, 0.05f, -0.05f),
                      glm::vec3(0.16f, 0.42f, 0.42f), caseBlack);

    // Bàn phím và chuột trên mặt bàn, nằm ngang theo Z (bảo vệ nhìn -X)
    renderer_.drawBox(deskPos + glm::vec3(-0.08f, 0.76f, -0.02f),
                      glm::vec3(0.15f, 0.015f, 0.44f), glm::vec3(0.16f, 0.16f, 0.18f)); // bàn phím
    renderer_.drawBox(deskPos + glm::vec3(-0.08f, 0.76f, 0.26f),
                      glm::vec3(0.11f, 0.015f, 0.07f), glm::vec3(0.16f, 0.16f, 0.18f)); // chuột
}

void GuardStation::drawDualMonitors(float floorY) const {
    // Màn hình hướng về +X (mặt screen hướng +X).
    // Bảo vệ ngồi phía +X nhìn về -X => bezel dày theo trục X (0.025f × 0.26f × 0.38f).
    const float deskTopY = floorY + islandHeight_ + 0.74f;
    const glm::vec3 bezelColor(0.08f, 0.08f, 0.09f);
    const glm::vec3 screen1Color(0.15f, 0.35f, 0.45f); // Màn hình 1: LPR (xanh lam)
    const glm::vec3 screen2Color(0.18f, 0.28f, 0.22f); // Màn hình 2: CCTV (xanh lá)
    const glm::vec3 standColor(0.25f, 0.26f, 0.28f);

    // Màn hình 1 (trái về phía Z âm) - đặt gần phía xe vào
    const glm::vec3 m1Pos(center_.x - 0.10f, deskTopY, center_.z - 0.20f);
    // Chân đỡ
    renderer_.drawBox(m1Pos, glm::vec3(0.14f, 0.02f, 0.16f), standColor);
    renderer_.drawPipeY(m1Pos + glm::vec3(0.0f, 0.02f, 0.0f), 0.15f, 0.02f, standColor);
    // Bezel: mỏng theo X, rộng theo Y và Z (mặt screen hướng +X)
    renderer_.drawBox(m1Pos + glm::vec3(0.015f, 0.17f, 0.0f),
                      glm::vec3(0.025f, 0.26f, 0.38f), bezelColor);
    // Screen nằm sâu hơn 1.5cm về phía -X
    renderer_.drawBox(m1Pos + glm::vec3(0.005f, 0.17f, 0.0f),
                      glm::vec3(0.005f, 0.23f, 0.35f), screen1Color);

    // Màn hình 2 (phải về phía Z dương)
    const glm::vec3 m2Pos(center_.x - 0.10f, deskTopY, center_.z + 0.20f);
    renderer_.drawBox(m2Pos, glm::vec3(0.14f, 0.02f, 0.16f), standColor);
    renderer_.drawPipeY(m2Pos + glm::vec3(0.0f, 0.02f, 0.0f), 0.15f, 0.02f, standColor);
    renderer_.drawBox(m2Pos + glm::vec3(0.015f, 0.17f, 0.0f),
                      glm::vec3(0.025f, 0.26f, 0.38f), bezelColor);
    renderer_.drawBox(m2Pos + glm::vec3(0.005f, 0.17f, 0.0f),
                      glm::vec3(0.005f, 0.23f, 0.35f), screen2Color);
}

void GuardStation::drawGuardChair(float floorY) const {
    const float chairBaseY = floorY + islandHeight_;
    // Bảo vệ ngồi phía +X so với bàn, nhìn về -X (nhìn ra phía xe vào từ dốc).
    // Ghế đặt ở +X so với tâm bàn.
    const glm::vec3 chairPos(center_.x + 0.56f, chairBaseY, center_.z);

    const glm::vec3 woodBrown(0.48f, 0.28f, 0.14f);     // Màu gỗ nâu bóng
    const glm::vec3 cushionColor(0.20f, 0.18f, 0.16f);   // Đệm ngồi tối
    const glm::vec3 metalLeg(0.18f, 0.19f, 0.20f);

    // 4 chân ghế
    for (float dx : {-0.18f, 0.18f}) {
        for (float dz : {-0.18f, 0.18f}) {
            renderer_.drawPipeY(chairPos + glm::vec3(dx, 0.0f, dz), 0.44f, 0.018f, metalLeg);
        }
    }

    // Đệm ngồi
    renderer_.drawBox(chairPos + glm::vec3(0.0f, 0.44f, 0.0f),
                      glm::vec3(0.44f, 0.05f, 0.44f), cushionColor);

    // Lưng ghế: 2 trụ dọc + tấm ván, lưng ghế ở phía +X (phía sau lưng bảo vệ khi nhìn về -X)
    renderer_.drawPipeY(chairPos + glm::vec3(-0.18f, 0.48f, -0.18f), 0.46f, 0.018f, woodBrown);
    renderer_.drawPipeY(chairPos + glm::vec3(-0.18f, 0.48f,  0.18f), 0.46f, 0.018f, woodBrown);
    renderer_.drawBox(chairPos + glm::vec3(-0.18f, 0.70f, 0.0f),
                      glm::vec3(0.025f, 0.22f, 0.38f), woodBrown);
    // Chi tiết trang trí lưng ghế
    renderer_.drawBox(chairPos + glm::vec3(-0.185f, 0.70f, 0.0f),
                      glm::vec3(0.03f, 0.12f, 0.14f), glm::vec3(0.25f, 0.14f, 0.07f));
}

void GuardStation::drawBarrierGate(const glm::vec3& postPos, float armLength, float openTimer, bool isEntry) const {
    const bool isOpen = openTimer > 0.0f;
    // Barrier arm angle: rotates upward 85° when opened.
    // Arm lies along +Z or -Z; rotating +X lifts it up toward +Y.
    const float targetAngle = isOpen ? 1.5708f : 0.0f; // +π/2 = arm straight up

    // Colors matching modern parking systems in AEON Mall & reference photos:
    const glm::vec3 safetyOrange(0.96f, 0.48f, 0.05f); // Vivid RAL 2000 orange housing
    const glm::vec3 capLightGrey(0.82f, 0.85f, 0.88f); // High-grade aluminum cap
    const glm::vec3 pivotWhite(0.95f, 0.95f, 0.96f);   // White circular pivot disc
    const glm::vec3 armWhite(0.96f, 0.96f, 0.95f);     // White aluminum boom arm
    const glm::vec3 stripeRed(0.88f, 0.10f, 0.08f);    // Reflective red hazard stripes

    // 1. Automatic Barrier Gate Housing (0.34m x 1.05m x 0.32m)
    renderer_.drawBox(postPos,
                      glm::vec3(0.34f, 1.00f, 0.32f), safetyOrange);

    // Top cap with bevel
    renderer_.drawBox(postPos + glm::vec3(0.0f, 1.00f, 0.0f),
                      glm::vec3(0.36f, 0.06f, 0.34f), capLightGrey);

    // Indicator LED strip on housing (Green when open, Red when closed)
    const glm::vec3 ledColor = isOpen ? glm::vec3(0.10f, 0.95f, 0.20f)
                                      : glm::vec3(0.95f, 0.10f, 0.08f);
    renderer_.drawBox(postPos + glm::vec3(0.0f, 0.92f, 0.165f),
                      glm::vec3(0.18f, 0.04f, 0.02f), ledColor);

    // 2. Circular Pivot disc on the side facing the lane
    const float sideOffset = isEntry ? -0.17f : 0.17f;
    const glm::vec3 pivotPos = postPos + glm::vec3(0.0f, 0.82f, sideOffset);

    // 3D round pivot disc
    renderer_.drawPipeZ(pivotPos, 0.04f, 0.14f, pivotWhite);

    // 3. Articulated boom arm with Red/White retro-reflective stripes
    const glm::mat4 pivotMat =
        glm::translate(glm::mat4(1.0f), pivotPos) *
        glm::rotate(glm::mat4(1.0f), targetAngle, glm::vec3(1.0f, 0.0f, 0.0f));

    // Arm direction vector: reaches outward into the traffic lane
    const float armSign = isEntry ? -1.0f : 1.0f;
    const glm::mat4 armMat =
        pivotMat *
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, armSign * armLength * 0.5f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.08f, 0.09f, armLength));

    renderer_.draw(renderer_.boxMesh(), armMat, armWhite);

    // Alternating reflective red stripes along the arm
    const int stripeCount = 7;
    const float stripeLen = armLength / static_cast<float>(stripeCount);
    for (int s = 0; s < stripeCount; s += 2) {
        const float zOffset = armSign * (static_cast<float>(s) + 0.5f) * stripeLen;
        const glm::mat4 stripeMat =
            pivotMat *
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.001f, zOffset)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.084f, 0.094f, stripeLen * 0.85f));
        renderer_.draw(renderer_.boxMesh(), stripeMat, stripeRed);
    }

    // Soft rubber bottom impact protection pad along the boom arm
    const glm::mat4 rubberMat =
        pivotMat *
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.045f, armSign * armLength * 0.5f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.04f, 0.02f, armLength));
    renderer_.draw(renderer_.boxMesh(), rubberMat, glm::vec3(0.12f, 0.13f, 0.14f));
}

void GuardStation::drawSpeedBumps(float floorY) const {
    const glm::vec3 rubberYellow(0.96f, 0.78f, 0.05f);
    const glm::vec3 rubberBlack(0.10f, 0.11f, 0.12f);

    // Inbound lane speed bump (placed right before the entry barrier)
    const float entryBumpX = entryBarrierX_ - 0.80f;
    const float exitBumpX  = exitBarrierX_ + 0.80f;

    auto drawBumpStrip = [&](float x, float zStart, float zEnd) {
        const float widthZ = std::abs(zEnd - zStart);
        const float segWidth = 0.35f;
        const int count = static_cast<int>(widthZ / segWidth);
        const float minZ = std::min(zStart, zEnd);

        for (int i = 0; i < count; ++i) {
            const float z = minZ + (static_cast<float>(i) + 0.5f) * segWidth;
            const glm::vec3 col = (i % 2 == 0) ? rubberYellow : rubberBlack;
            // 3D curved rubber bump profile
            renderer_.drawBox(glm::vec3(x, floorY, z),
                              glm::vec3(0.38f, 0.055f, segWidth), col);
        }
    };

    // Inbound lane bump (Z: -3.4m to -0.6m)
    drawBumpStrip(entryBumpX, -3.40f, -0.60f);

    // Outbound lane bump (Z: +0.6m to +3.4m)
    drawBumpStrip(exitBumpX, 0.60f, 3.40f);
}

void GuardStation::drawOverheadCameras(float floorY) const {
    const float ceilingY = floorY + AeonLayout::FLOOR_TO_CEILING_M;
    const glm::vec3 camWhite(0.92f, 0.93f, 0.95f);
    const glm::vec3 lensBlack(0.05f, 0.05f, 0.06f);
    const glm::vec3 bracketColor(0.35f, 0.38f, 0.40f);

    auto drawCamera = [&](const glm::vec3& topPos, float yawAngle, float pitchAngle) {
        // Vertical steel drop pipe from ceiling
        renderer_.drawPipeY(topPos - glm::vec3(0.0f, 0.70f, 0.0f), 0.70f, 0.018f, bracketColor);

        // Mounting bracket pivot
        const glm::vec3 pivot = topPos - glm::vec3(0.0f, 0.70f, 0.0f);
        renderer_.drawBox(pivot, glm::vec3(0.08f, 0.08f, 0.08f), bracketColor);

        // Cylindrical bullet camera body angled toward the approaching car
        const glm::mat4 camModel =
            glm::translate(glm::mat4(1.0f), pivot) *
            glm::rotate(glm::mat4(1.0f), yawAngle, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(glm::mat4(1.0f), pitchAngle, glm::vec3(0.0f, 0.0f, 1.0f));

        const glm::mat4 bodyMat =
            camModel *
            glm::translate(glm::mat4(1.0f), glm::vec3(0.14f, -0.05f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.32f, 0.12f, 0.12f));
        renderer_.draw(renderer_.boxMesh(), bodyMat, camWhite);

        // Camera lens / sunshade
        const glm::mat4 lensMat =
            camModel *
            glm::translate(glm::mat4(1.0f), glm::vec3(0.30f, -0.05f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.10f, 0.10f));
        renderer_.draw(renderer_.boxMesh(), lensMat, lensBlack);
    };

    // Camera 1 & 2 for Inbound Lane: suspended right above entrance checkpoint,
    // angled downward to capture front license plate and driver cabin
    drawCamera(glm::vec3(entryBarrierX_ + 0.40f, ceilingY, -1.80f), 3.14159f, 0.40f);
    drawCamera(glm::vec3(entryBarrierX_ + 0.40f, ceilingY, -0.80f), 3.14159f, 0.40f);

    // Camera 3 & 4 for Outbound Lane: capturing rear plate as cars exit to ramp
    drawCamera(glm::vec3(exitBarrierX_ - 0.40f, ceilingY, 1.80f), 0.0f, 0.40f);
    drawCamera(glm::vec3(exitBarrierX_ - 0.40f, ceilingY, 0.80f), 0.0f, 0.40f);
}

void GuardStation::drawLaneDirectionArrows(float floorY) const {
    const glm::vec3 arrowYellow(0.96f, 0.78f, 0.05f);
    const float y = floorY + 0.0002f;

    // Keep the two-metre post-gate lead-in visibly straight before the fork.
    const float inX = entryBarrierX_ + AeonLayout::GATE_POST_PASS_STRAIGHT_M * 0.5f;
    const float inZ = AeonLayout::ENTRY_LANE_CENTER_Z_M;
    renderer_.drawFlatXZ(glm::vec3(inX, y, inZ),
                         glm::vec2(1.40f, 0.22f), arrowYellow);
    renderer_.drawFlatXZRotated(glm::vec3(inX + 0.70f, y, inZ - 0.18f),
                                glm::vec2(0.50f, 0.18f), -0.55f, arrowYellow);
    renderer_.drawFlatXZRotated(glm::vec3(inX + 0.70f, y, inZ + 0.18f),
                                glm::vec2(0.50f, 0.18f), 0.55f, arrowYellow);

    auto drawZArrow = [&](float z, float direction) {
        renderer_.drawFlatXZ(glm::vec3(AeonLayout::CONNECTOR_X_M, y, z),
                             glm::vec2(0.18f, 1.05f), arrowYellow);
        for (int side : {-1, 1}) {
            renderer_.drawFlatXZRotated(
                glm::vec3(AeonLayout::CONNECTOR_X_M + static_cast<float>(side) * 0.18f,
                          y, z + direction * 0.62f),
                glm::vec2(0.48f, 0.16f),
                static_cast<float>(side) * 0.62f, arrowYellow);
        }
    };

    // At the junction, mark both choices toward the two parking aisles.
    drawZArrow(-3.50f, -1.0f);
    drawZArrow(0.00f, 1.0f);

    // Arrow on the separate outbound lane: points toward the ramp (-X).
    const float outX = exitBarrierX_ - 1.20f;
    const float outZ = AeonLayout::EXIT_LANE_CENTER_Z_M;
    renderer_.drawFlatXZ(glm::vec3(outX, y, outZ),
                         glm::vec2(1.40f, 0.22f), arrowYellow);
    renderer_.drawFlatXZRotated(glm::vec3(outX - 0.70f, y, outZ - 0.18f),
                                glm::vec2(0.50f, 0.18f), 0.55f, arrowYellow);
    renderer_.drawFlatXZRotated(glm::vec3(outX - 0.70f, y, outZ + 0.18f),
                                glm::vec2(0.50f, 0.18f), -0.55f, arrowYellow);
}
