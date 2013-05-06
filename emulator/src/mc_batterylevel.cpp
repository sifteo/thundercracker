#include <sifteo/abi.h>
#include "batterylevel.h"
#include "macros.h"

namespace BatteryLevel {

void updatePercentage(int delta) // percentage
{
    percentage = clamp(int(percentage + delta), 0, 100);
}

unsigned getPercentage()
{
    return percentage;
}

unsigned scaled()
{
    return percentage * _SYS_BATTERY_MAX / 100;
}

void heartbeat()
{
    static unsigned beatDivider = 0;

    if (++beatDivider == 10) {  // check every second (heartbeat = 10Hz)
        beatDivider = 0;
        onCapture();            // trigger the warning flags
    }
}

} // namespace BatteryLevel
