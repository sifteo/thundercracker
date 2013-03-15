#include "batterylevel.h"
#include "pause.h"
#include "tasks.h"

namespace BatteryLevel {

bool aDeviceIsLow()
{
	// was there a device with a low battery level ?
    return getNextLowBatDevice() != NONE;
}

void setWarningCompleted()
{
    selectedCube = NONE;
}

void onCapture(uint32_t batLevel, _SYSCubeID cid)
{
    ASSERT(cid <= BASE);

    if (!Pause::finished)
        return;

    // trigger a warning (once) if 90% discharged
    if (canWarn.test(cid) && batLevel <= _SYS_BATTERY_MAX/10) {
        if (selectedCube == NONE) {
            setSelectedCube(cid);
            Pause::taskWork.atomicMark(Pause::LowBattery);
            Tasks::trigger(Tasks::Pause);
        } else {
            lowBatDevices.atomicMark(cid);
        }
    }
}

_SYSCubeID getNextLowBatDevice()
{
    if (selectedCube != NONE) {
        return selectedCube;
    }

    unsigned index;
    if (lowBatDevices.findFirst(index)) {
        setSelectedCube(index);
        lowBatDevices.atomicClear(index);
        return uint8_t(selectedCube);
    }

    return NONE;
}

void setSelectedCube(_SYSCubeID cid)
{
    ASSERT(cid <= BASE);

    selectedCube = cid;
    canWarn.atomicClear(cid);
}

} // namespace BatteryLevel
