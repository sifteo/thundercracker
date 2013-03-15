#include "batterymonitor.h"
#include "pause.h"
#include "tasks.h"

namespace BatteryMonitor {

void init()
{
    /*
     * These bit vectors tell which devices:
     *   - can display a warning
     *   - need to display a warning
     */

    canWarn.mark();
    lowBatDevices.clear();
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

bool aDeviceIsLow()
{
	// was there a device with a low battery level ?
    return getNextLowBatDevice() != NONE;
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

void setWarningCompleted()
{
    selectedCube = NONE;
}

} // namespace BatteryLevel
