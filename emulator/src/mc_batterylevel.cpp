#include <sifteo/abi.h>
#include "batterylevel.h"
#include "macros.h"

namespace BatteryLevel {

void updatePercentage(int delta, unsigned cubeNum)
{
    percentage[cubeNum] = clamp(percentage[cubeNum] + delta, 0, 100);
}

unsigned getPercentage(unsigned cubeNum)
{
    return percentage[cubeNum];
}

unsigned scaled(unsigned cubeNum) // base by default
{
    return percentage[cubeNum] * _SYS_BATTERY_MAX / 100;
}

void heartbeat()
{
    // It could be done in updatePercentage() but this system allows
    // to leave a little break between different pause/warnings.
    static unsigned beatDivider = 0;

    if (++beatDivider == 10) {  // check every second (called @ 10Hz)
        beatDivider = 0;
        onCapture();            // trigger the warning flags
    }
}

void init()
{
    needWarningVect.clear();
    // Initialize default values of simulated battery levels
    for (int i = 0; i <= _SYS_NUM_CUBE_SLOTS; i++)
        percentage[i] = 100;
}

} // namespace BatteryLevel
