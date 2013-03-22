#include <sifteo/abi.h>
#include "batterylevel.h"
#include "macros.h"

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

unsigned scaled()
{
    return percentage * _SYS_BATTERY_MAX / 100;
}

void heartbeat()
{
    // simulate the regular capture made in real hardware
    static uint8_t beatDivider = 0;

    if (++beatDivider == 10) {  // check every second (called @ 10Hz)
        beatDivider = 0;

        // trigger the warning flags if needed (for the base)
        BatteryMonitor::onCapture(scaled(), BatteryMonitor::BASE);
    }
}

} // namespace BatteryLevel
