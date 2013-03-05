#include "batterylevel.h"
#include "pause.h"
#include "tasks.h"

namespace BatteryLevel {

bool needWarning()
{
    return !lowBatDevices.empty(); // was there a device with a low battery level ?
}

void setWarningDone(uint8_t cubeNum)
{
    warningDone.atomicMark(cubeNum);
    lowBatDevices.atomicClear(cubeNum);
}

void onCapture(uint32_t batLevel, uint8_t cubeNum)
{
    // update the lowBatDevices, trigger a warning (once) if 90% discharged
    if (!warningDone.test(cubeNum) && batLevel <= _SYS_BATTERY_MAX/10) {
        lowBatDevices.atomicMark(cubeNum);
        Pause::taskWork.atomicMark(Pause::LowBattery);
        Tasks::trigger(Tasks::Pause);
    }
}

uint8_t getLowBatDevice()
{
    unsigned index = 0;
    bool needWarning = lowBatDevices.findFirst(index);
    ASSERT(needWarning); // needWarning() should be called before
    return uint8_t(index);
}

} // namespace BatteryLevel
