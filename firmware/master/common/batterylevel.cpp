#include "batterylevel.h"
#include "pause.h"
#include "tasks.h"

namespace BatteryLevel {


bool needWarning()
{
    return (neverWarned && _SYS_sysBatteryLevel() <= _SYS_BATTERY_MAX/10);
}

void setWarningDone()
{
    neverWarned = false;
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
