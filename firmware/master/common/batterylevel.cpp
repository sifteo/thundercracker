#include "batterylevel.h"
#include "pause.h"
#include "tasks.h"
#include "cubeslots.h"

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

void onCapture() // update lowBatDevice and trigger a warning (once) if 90% discharge
{
    lowBatDevice = NONE;

    if (!warningDone.test(BASE) && _SYS_sysBatteryLevel() <= _SYS_BATTERY_MAX/10) {
        lowBatDevice = BASE;
    } else {
        _SYSCubeIDVector connectedCubes = CubeSlots::userConnected;
        while (connectedCubes && lowBatDevice == NONE) {
            uint8_t i = Intrinsic::CLZ(connectedCubes); // get first connected cube number
            connectedCubes ^= Intrinsic::LZ(i);         // mark it as read
            if (!warningDone.test(i) && _SYS_cubeBatteryLevel(i) <= _SYS_BATTERY_MAX/10) {
                lowBatDevice = i;
            }
        }
    }
    if (lowBatDevice != NONE) {
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
