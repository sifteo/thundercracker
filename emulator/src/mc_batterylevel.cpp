#include <sifteo/abi.h>
#include "batterylevel.h"
#include "macros.h"
#include "cubeslots.h"

namespace BatteryLevel {

// In the following array, the last element refers to the master cube:
static uint8_t percentage[_SYS_NUM_CUBE_SLOTS+1] = {0}; // will be 100% in init()


void updatePercentage(int8_t delta, uint8_t cubeNum)
{
    percentage[cubeNum] = clamp(percentage[cubeNum] + delta, 0, 100);
}

uint8_t getPercentage(uint8_t cubeNum)
{
    return percentage[cubeNum];
}

unsigned scaled(uint8_t cubeNum) // base by default
{
    return percentage[cubeNum] * _SYS_BATTERY_MAX / 100;
}

void heartbeat()
{
    // simulate the regular capture made in real hardware
    static uint8_t beatDivider = 0;

    if (++beatDivider == 10) {  // check every second (called @ 10Hz)
        beatDivider = 0;
        // trigger the warning flags if needed, for the base...
        onCapture(_SYS_sysBatteryLevel(), BatteryLevel::BASE);
        // ...and for the cubes:
        _SYSCubeIDVector connectedCubes = CubeSlots::userConnected;
        while (lowBatDevice == NONE && connectedCubes) {
            uint8_t cid = Intrinsic::CLZ(connectedCubes); // get first connected cube number
            connectedCubes ^= Intrinsic::LZ(cid);         // mark it as read
            onCapture(_SYS_cubeBatteryLevel(cid), cid);
        }
    }
}

void init()
{
    warningDone.clear();
    // Initialize default values of simulated battery levels
    for (int i = 0; i <= _SYS_NUM_CUBE_SLOTS; i++) {
        percentage[i] = 100;
    }
}

} // namespace BatteryLevel
