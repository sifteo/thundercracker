#include <sifteo/abi.h>
#include "batterylevel.h"
#include "macros.h"
#include "cubeslots.h"
#include "frontend.h"

namespace BatteryLevel {

// Base battery level
static uint8_t percentage = 42; // because it's THE answer

void updatePercentage(int8_t delta)
{
    percentage = clamp(percentage + delta, 0, 100);
}

uint8_t getPercentage()
{
    return percentage;
}

unsigned scaled(_SYSCubeID cid) // base by default
{
    ASSERT(cid <= BASE);

    if (cid == BASE) {
        return percentage * _SYS_BATTERY_MAX / 100;
    } else {
        FrontendCube& cube = Frontend::getCube(cid);
        return cube.getBattery();
    }
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
            // get first connected cube number and mark it as read
            _SYSCubeID cid = Intrinsic::CLZ(connectedCubes);
            connectedCubes ^= Intrinsic::LZ(cid);
            onCapture(scaled(cid), cid);
        }
    }
}

void init()
{
    /*
     * These bit vectors tell which devices:
     *   - can display a warning
     *   - need to display a warning
     */

    canWarn.mark();
    lowBatDevices.clear();
}

} // namespace BatteryLevel
