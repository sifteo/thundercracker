#include "batterylevel.h"
#include "pause.h"
#include "tasks.h"

namespace BatteryLevel {

bool needWarning()
{
    return (lowBatDevice != NONE); // was there a device with a low battery level ?
}

void setWarningDone(uint8_t cubeNum)
{
    warningDone.atomicMark(cubeNum);
    lowBatDevice = NONE; // useful if call needWarning() before update
}

void onCapture(uint32_t batLevel, uint8_t cubeNum)
{
    // update the lowBatDevice number and trigger a warning (once) if 90% discharge
    if (lowBatDevice != NONE) { // do we already have a warning to display ?
        return;
    }
    lowBatDevice = NONE;

    if (!warningDone.test(cubeNum) && batLevel <= _SYS_BATTERY_MAX/10) {
        lowBatDevice = cubeNum;
        Pause::taskWork.atomicMark(Pause::LowBattery);
        Tasks::trigger(Tasks::Pause);
    }
}

uint8_t getLowBatDevice()
{
    ASSERT(lowBatDevice != NONE); // needWarning() should be called before
    return lowBatDevice;
}

} // namespace BatteryLevel
