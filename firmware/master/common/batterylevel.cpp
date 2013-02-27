#include "batterylevel.h"
#include "pause.h"
#include "tasks.h"
#include "cubeslots.h"

namespace BatteryLevel {

bool needWarning()
{
    return (lowBatDevice != NONE); // was there a device with a low battery level ?
}

void setWarningDone(unsigned cubeNum)
{
    warningDone.atomicMark(cubeNum);
    lowBatDevice = NONE; // useful if call needWarning() before update
}

void onCapture() // update lowBatDevice and trigger a warning (once) if 90% discharge
{
    lowBatDevice = NONE;

    if (!warningDone.test(BASE) && _SYS_sysBatteryLevel() <= _SYS_BATTERY_MAX/10) {
        lowBatDevice = BASE;
    } else {
        for (_SYSCubeID i = 0; i < CubeSlots::maxUserCubes; i++) {
            if (!warningDone.test(i) && _SYS_cubeBatteryLevel(i) <= _SYS_BATTERY_MAX/10) {
                lowBatDevice = i;
                break;
            }
        }
    }
    if (lowBatDevice != NONE) {
        Pause::taskWork.atomicMark(Pause::LowBattery);
        Tasks::trigger(Tasks::Pause);
    }
}

unsigned getLowBatDevice()
{
    return lowBatDevice;
}

} // namespace BatteryLevel
