#pragma once
// ============================================================================
//  vehicle_models.h  -  vehicle types available in the parking simulation.
//
//  GL-free on purpose: the world model (parking_world.h) only needs the enum,
//  and the renderer turns the "part tables" below into boxes and cylinders.
//  Local frame (same as car_model.h): +X forward, +Y up, +Z = car's right,
//  origin on the ground at the middle of the car, metres.
//
//  HOW TO ADD ANOTHER CAR
//    1. Add a value to `Model` (and bump MODEL_COUNT).
//    2. Add a `xxxParts()` table (copy vf3Parts() and change the numbers).
//    3. Return it from `partsFor()` and decide in `modelForVehicleId()` which
//       cars use it. Nothing else changes.
//
//  Dimensions follow public manufacturer figures but the shapes are simplified
//  procedural stand-ins - not scanned CAD models. Track width and wheel radius
//  are [ASSUMED].
// ============================================================================
#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace vehicle_models {

enum class Model {
    ToyotaVios = 0,  // lofted mesh from car_model.h (drawn by Renderer::drawCar)
    VinFastVF3 = 1,  // boxy micro-SUV built from the part table below
};
inline constexpr int MODEL_COUNT = 2;

// Public body dimensions in metres.
struct Spec {
    const char* name;
    float length, width, height, wheelbase;
};

inline Spec specFor(Model model) {
    switch (model) {
    case Model::VinFastVF3: return {"VinFast VF 3", 3.190f, 1.679f, 1.622f, 2.075f};
    case Model::ToyotaVios: break;
    }
    return {"Toyota Vios", 4.41f, 1.74f, 1.48f, 2.55f};
}

// Which car type the n-th spawned car is. Deterministic (no random numbers) so
// simulation tests stay repeatable: odd ids are VF 3, even ids are Vios.
inline Model modelForVehicleId(int id) {
    return (id % 2 != 0) ? Model::VinFastVF3 : Model::ToyotaVios;
}

// Paint used for ordinary traffic. Vios keeps the original silver; VF 3 cycles
// through a few of its factory-style colours (the first one is white).
inline glm::vec3 bodyColorForVehicle(Model model, int id) {
    if (model != Model::VinFastVF3) return glm::vec3(0.72f, 0.74f, 0.77f);
    static const glm::vec3 palette[4] = {
        glm::vec3(0.94f, 0.95f, 0.96f),  // white
        glm::vec3(0.96f, 0.80f, 0.12f),  // yellow
        glm::vec3(0.45f, 0.72f, 0.90f),  // sky blue
        glm::vec3(0.60f, 0.62f, 0.65f),  // grey
    };
    return palette[(id / 2) % 4];
}

// --------------------------------------------------------------- part tables
enum class PartKind { Box, Cylinder };

struct Part {
    PartKind kind = PartKind::Box;
    // Box:      a = centre of the bottom face, b = size (x, y, z).
    // Cylinder: a = start point, b = end point (axis), `radius` = radius.
    glm::vec3 a{0.0f};
    glm::vec3 b{0.0f};
    float radius = 0.0f;
    float tiltZ = 0.0f;       // Box only: lean about local Z (radians, + leans backwards)
    bool useBodyColor = false;
    glm::vec3 color{0.0f};
};

namespace detail {
inline void box(std::vector<Part>& parts, float cx, float y0, float cz,
                float sx, float sy, float sz, const glm::vec3& color,
                bool bodyColor = false, float tilt = 0.0f) {
    Part p;
    p.kind = PartKind::Box;
    p.a = glm::vec3(cx, y0, cz);
    p.b = glm::vec3(sx, sy, sz);
    p.tiltZ = tilt;
    p.useBodyColor = bodyColor;
    p.color = color;
    parts.push_back(p);
}
// Same box on both sides of the car (z and -z).
inline void boxPair(std::vector<Part>& parts, float cx, float y0, float cz,
                    float sx, float sy, float sz, const glm::vec3& color,
                    bool bodyColor = false) {
    box(parts, cx, y0, cz, sx, sy, sz, color, bodyColor);
    box(parts, cx, y0, -cz, sx, sy, sz, color, bodyColor);
}
inline void cylinder(std::vector<Part>& parts, const glm::vec3& from, const glm::vec3& to,
                     float radius, const glm::vec3& color) {
    Part p;
    p.kind = PartKind::Cylinder;
    p.a = from;
    p.b = to;
    p.radius = radius;
    p.color = color;
    parts.push_back(p);
}
} // namespace detail

// VinFast VF 3: 3.190 x 1.679 x 1.622 m, wheelbase 2.075 m, two doors, tall
// square greenhouse, black wheel-arch cladding and black rear quarter glass.
inline const std::vector<Part>& vf3Parts() {
    static const std::vector<Part> parts = [] {
        std::vector<Part> p;
        using detail::box;
        using detail::boxPair;
        const glm::vec3 black(0.07f, 0.07f, 0.08f);
        const glm::vec3 glassDark(0.06f, 0.08f, 0.10f);
        const glm::vec3 glassSide(0.20f, 0.23f, 0.27f);
        const glm::vec3 seam(0.58f, 0.60f, 0.63f);
        const glm::vec3 headLamp(0.97f, 0.96f, 0.88f);
        const glm::vec3 tailLamp(0.70f, 0.14f, 0.08f);
        const glm::vec3 tire(0.06f, 0.06f, 0.07f);
        const glm::vec3 hub(0.16f, 0.16f, 0.18f);

        const float axleFront = 1.0375f;   // wheelbase 2.075 m
        const float axleRear = -1.0375f;
        const float wheelZ = 0.73f;        // [ASSUMED] track 1.46 m
        const float wheelR = 0.32f;        // [ASSUMED] ~16 in wheel with 175/75 tyre
        const float wheelHalfW = 0.0925f;
        const float archHalf = 0.43f;      // wheel-arch half opening
        const float archTop = 0.74f;

        // ---- main body (painted)
        box(p, 0.0f, archTop, 0.0f, 3.02f, 0.31f, 1.60f, glm::vec3(0.0f), true);      // belt band y 0.74..1.05
        box(p, 0.0f, 0.30f, 0.0f, 1.21f, archTop - 0.30f, 1.60f, glm::vec3(0.0f), true); // between arches
        box(p, -1.065f, 1.05f, 0.0f, 0.89f, 0.25f, 1.60f, glm::vec3(0.0f), true);     // rear shoulder y 1.05..1.30
        box(p, -0.50f, 1.50f, 0.0f, 1.90f, 0.10f, 1.56f, glm::vec3(0.0f), true);      // roof y 1.50..1.60

        // ---- black cladding
        box(p, 0.0f, 0.22f, 0.0f, 1.21f, 0.16f, 1.64f, black);                         // sills
        box(p, 1.50f, 0.28f, 0.0f, 0.19f, archTop - 0.28f, 1.62f, black);              // front bumper
        box(p, -1.50f, 0.28f, 0.0f, 0.19f, archTop - 0.28f, 1.62f, black);             // rear bumper
        for (float ax : {axleFront, axleRear}) {
            box(p, ax, 0.24f, 0.0f, archHalf * 2.0f, archTop - 0.24f, 1.30f, black);   // wheel well
            for (float s : {1.0f, -1.0f}) {
                box(p, ax, archTop, s * 0.77f, archHalf * 2.0f + 0.14f, 0.13f, 0.14f, black); // arch flare
                box(p, ax + archHalf + 0.035f, 0.30f, s * 0.77f, 0.07f, archTop - 0.30f, 0.14f, black);
                box(p, ax - archHalf - 0.035f, 0.30f, s * 0.77f, 0.07f, archTop - 0.30f, 0.14f, black);
            }
        }

        // ---- glasshouse
        box(p, -0.49f, 1.05f, 0.0f, 1.82f, 0.45f, 1.48f, glassSide);                   // side glass volume y 1.05..1.50
        // Raked windscreen: a 0.40 m thick slab leaning back 42.7 deg. Its centre is set
        // back half the thickness so the front face passes through (x 0.80, y 1.05).
        box(p, 0.653f, 0.914f, 0.0f, 0.40f, 0.62f, 1.44f, glassDark, false, 0.745f);
        boxPair(p, -1.01f, 1.30f, 0.745f, 0.78f, 0.20f, 0.02f, black);                 // rear quarter glass
        box(p, -0.62f, 1.05f, 0.0f, 0.09f, 0.45f, 1.50f, black);                       // B pillar
        box(p, -1.33f, 1.60f, 0.0f, 0.14f, 0.022f, 0.40f, black);                      // roof fin

        // ---- details
        boxPair(p, 0.62f, 0.38f, 0.802f, 0.012f, 0.67f, 0.004f, seam);                 // front door seam
        boxPair(p, -0.67f, 0.38f, 0.802f, 0.012f, 0.67f, 0.004f, seam);                // rear door seam
        boxPair(p, -0.49f, 0.95f, 0.808f, 0.14f, 0.03f, 0.016f, black);                // door handle
        boxPair(p, 0.45f, 1.12f, 0.80f, 0.08f, 0.14f, 0.12f, black);                   // mirror
        boxPair(p, 1.52f, 0.84f, 0.52f, 0.04f, 0.12f, 0.34f, headLamp);                // head lamp
        box(p, 1.525f, 0.93f, 0.0f, 0.03f, 0.03f, 1.12f, headLamp);                    // LED bar
        boxPair(p, -1.52f, 0.86f, 0.62f, 0.04f, 0.16f, 0.20f, tailLamp);               // tail lamp

        // ---- wheels
        for (float ax : {axleFront, axleRear}) {
            for (float s : {1.0f, -1.0f}) {
                const float z = s * wheelZ;
                detail::cylinder(p, glm::vec3(ax, wheelR, z - wheelHalfW),
                                 glm::vec3(ax, wheelR, z + wheelHalfW), wheelR, tire);
                detail::cylinder(p, glm::vec3(ax, wheelR, z - wheelHalfW - 0.008f),
                                 glm::vec3(ax, wheelR, z + wheelHalfW + 0.008f), 0.17f, hub);
            }
        }
        return p;
    }();
    return parts;
}

// Part table for a model; empty means "drawn by a dedicated mesh" (Vios).
inline const std::vector<Part>& partsFor(Model model) {
    static const std::vector<Part> none;
    return model == Model::VinFastVF3 ? vf3Parts() : none;
}

// Axis-aligned bounds of a part table in the local car frame (for tests and
// for checking a new model fits inside the simulation footprint).
struct Bounds {
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f, minZ = 1e9f, maxZ = -1e9f;
    void add(float x, float y, float z) {
        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
        minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);
    }
};

inline Bounds measure(const std::vector<Part>& parts) {
    Bounds bounds;
    for (const Part& part : parts) {
        if (part.kind == PartKind::Cylinder) {
            // A disc of radius r reaches r*sqrt(1 - d_axis^2) along each world axis.
            const glm::vec3 axis = part.b - part.a;
            const float len = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
            const float dx = len > 0.0f ? axis.x / len : 0.0f;
            const float dy = len > 0.0f ? axis.y / len : 0.0f;
            const float dz = len > 0.0f ? axis.z / len : 0.0f;
            const float ex = part.radius * std::sqrt(std::max(0.0f, 1.0f - dx * dx));
            const float ey = part.radius * std::sqrt(std::max(0.0f, 1.0f - dy * dy));
            const float ez = part.radius * std::sqrt(std::max(0.0f, 1.0f - dz * dz));
            for (const glm::vec3& end : {part.a, part.b}) {
                bounds.add(end.x - ex, end.y - ey, end.z - ez);
                bounds.add(end.x + ex, end.y + ey, end.z + ez);
            }
            continue;
        }
        const float c = std::cos(part.tiltZ);
        const float s = std::sin(part.tiltZ);
        for (int ix = 0; ix < 2; ++ix) {
            for (int iy = 0; iy < 2; ++iy) {
                for (int iz = 0; iz < 2; ++iz) {
                    const float x = (ix == 0 ? -0.5f : 0.5f) * part.b.x;
                    const float y = (iy == 0 ? 0.0f : 1.0f) * part.b.y;
                    const float z = (iz == 0 ? -0.5f : 0.5f) * part.b.z;
                    bounds.add(part.a.x + x * c - y * s, part.a.y + x * s + y * c, part.a.z + z);
                }
            }
        }
    }
    return bounds;
}

} // namespace vehicle_models
