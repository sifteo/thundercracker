#include "batterylevel.h"
#include "pause.h"
#include "tasks.h"
#ifdef SIFTEO_SIMULATOR
#include "frontend.h"
#endif

namespace BatteryLevel {

void updatePercentage(int delta) // percentage
{
    percentage = clamp(int(percentage + delta), 0, 100);
}

unsigned getPercentage()
{
    return percentage;
}

bool needWarning()
{
    return (neverWarned && _SYS_sysBatteryLevel() <= _SYS_BATTERY_MAX/10);
}

void setWarningDone()
{
    neverWarned = false;
}

void heartbeat()
{
    static unsigned beatDivider = 0;

    if (++beatDivider == 10) // check every second (heartbeat = 10Hz)
    {
        beatDivider = 0;
        onCapture();
    }
}

void onCapture() // warn the user (once) when reach 90% discharge
{
    if (needWarning())
    {
        Pause::taskWork.atomicMark(Pause::LowBattery);
        Tasks::trigger(Tasks::Pause);
    }
}


} // namespace BatteryLevel
