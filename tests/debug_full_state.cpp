#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include "../parking_world.h"

int main() {
    ParkingWorld world(AeonLayout::FLOOR_HEIGHT_B1_M, AeonLayout::FLOOR_INDEX_B1, 20261118u);
    world.initialise();

    int phaseCounts[static_cast<int>(Vehicle::Phase::ExitingRamp) + 1] = {0};
    const char* phaseNames[] = {
        "EnteringRamp", "EntryGateWait", "ToEntryConnector", "ToTargetAisle",
        "ToBayApproach", "ToBayMouth", "ToBayStaging", "ReversingIntoBay",
        "Parked", "LeavingBay", "AligningOutboundLane", "ToExitConnector",
        "ToExitLane", "ExitGateApproach", "ExitPaymentWait", "ExitingRamp"
    };

    for (int frame = 0; frame < 6000; ++frame) {
        world.update(0.04f);
        world.updateSpawning(0.04f);

        for (int i = 0; i <= static_cast<int>(Vehicle::Phase::ExitingRamp); ++i) phaseCounts[i] = 0;
        for (const auto& v : world.vehicles()) {
            phaseCounts[static_cast<int>(v.phase())]++;
        }

        if (frame % 250 == 0 || world.freeSlotCount() == 0) {
            std::cout << "Frame " << frame
                      << " | cars=" << world.carCount()
                      << " free=" << world.freeSlotCount()
                      << " res=" << world.reservedSlotCount()
                      << " occ=" << world.occupiedSlotCount()
                      << " | ";
            for (int i = 0; i <= static_cast<int>(Vehicle::Phase::ExitingRamp); ++i) {
                if (phaseCounts[i] > 0) {
                    std::cout << phaseNames[i] << "=" << phaseCounts[i] << " ";
                }
            }
            std::cout << "\n";
            if (world.freeSlotCount() == 0) {
                std::cout << "FULL reached at frame " << frame << "\n";
                break;
            }
        }
        if (world.hasTrafficCollisions()) {
            std::cout << "COLLISION at frame " << frame << "\n";
            break;
        }
    }
    return 0;
}
