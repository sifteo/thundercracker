#include <sifteo/abi.h>
#include "batterylevel.h"
#include "macros.h"
#include "cubeslots.h"

namespace BatteryLevel {

// In the following array, the last element refers to the master cube:
static uint8_t percentage[_SYS_NUM_CUBE_SLOTS+1];

void updatePercentage(int8_t delta, _SYSCubeID cid)
{
    ASSERT(cid <= BASE);

    percentage[cid] = clamp(percentage[cid] + delta, 0, 100);
}

uint8_t getPercentage(_SYSCubeID cid)
{
    ASSERT(cid <= BASE);

    return percentage[cid];
}

unsigned scaled(_SYSCubeID cid) // base by default
{
    ASSERT(cid <= BASE);

    return percentage[cid] * _SYS_BATTERY_MAX / 100;
}

void heartbeat()
{
    // simulate the regular capture made in real hardware
    static uint8_t beatDivider = 0;

    if (++beatDivider == 10) {  // check every second (called @ 10Hz)
        beatDivider = 0;

        // trigger the warning flags if needed, for the base...
        onCapture(scaled(), BatteryLevel::BASE);

        // ...and for the cubes:
        _SYSCubeIDVector connectedCubes = CubeSlots::userConnected;
        while (connectedCubes) {
            uint8_t cid = Intrinsic::CLZ(connectedCubes); // get first connected cube number
            connectedCubes ^= Intrinsic::LZ(cid);         // mark it as read
            onCapture(_SYS_cubeBatteryLevel(cid), cid);
        }
    }
}

void init()
{
    canWarn.clear();
    lowBatDevices.clear();
    // Initialize default values of simulated battery levels
    for (int i = 0; i <= _SYS_NUM_CUBE_SLOTS; i++) {
        percentage[i] = 100;
    }
}

} // namespace BatteryLevel
