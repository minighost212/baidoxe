#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include "../parking_world.h"

int main() {
    ParkingWorld world(AeonLayout::FLOOR_HEIGHT_B1_M, AeonLayout::FLOOR_INDEX_B1, 20261118u);
    world.initialise();

    for (int frame = 0; frame < 5000; ++frame) {
        world.update(0.04f);
        world.updateSpawning(0.04f);

        // Detect a vehicle stuck in ToBayMouth for many frames
        if (frame > 2000) {
            for (const auto& v : world.vehicles()) {
                if (v.phase() == Vehicle::Phase::ToBayMouth) {
                    const ParkingSlot& s = world.slots()[v.slotIndex()];
                    float stagingZ = s.aisleZ() + static_cast<float>(s.rowSide()) *
                        (AeonLayout::AISLE_W_M * 0.5f - AeonLayout::BAY_TURN_STAGING_OFFSET_M);
                    std::cout << "Frame " << frame
                              << " ToBayMouth car id=" << v.id()
                              << " slotIdx=" << v.slotIndex()
                              << " pos=(" << v.position().x << "," << v.position().z << ")"
                              << " target=(" << s.center().x << "," << stagingZ << ")"
                              << " aisleZ=" << s.aisleZ() << " rowSide=" << s.rowSide()
                              << " yaw=" << v.yaw() << "\n";
                    // List other vehicles that could block
                    for (const auto& o : world.vehicles()) {
                        if (o.id() == v.id()) continue;
                        float dx = std::abs(o.position().x - v.position().x);
                        float dz = std::abs(o.position().z - v.position().z);
                        if (dx < 6.0f && dz < 6.0f) {
                            std::cout << "   nearby id=" << o.id()
                                      << " phase=" << static_cast<int>(o.phase())
                                      << " pos=(" << o.position().x << "," << o.position().z << ")\n";
                        }
                    }
                    return 0;
                }
            }
        }
    }
    std::cout << "No stuck ToBayMouth found\n";
    return 0;
}
