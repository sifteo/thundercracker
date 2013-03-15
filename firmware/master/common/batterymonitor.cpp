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

void onCapture(uint32_t batLevel, _SYSDeviceID id)
{
    ASSERT(id <= BASE);

    if (!Pause::finished)
        return;

    // trigger a warning (once) if 90% discharged
    if (canWarn.test(id) && batLevel <= _SYS_BATTERY_MAX/10) {
        if (selectedDevice == NONE) {
            setSelectedDevice(id);
            Pause::taskWork.atomicMark(Pause::LowBattery);
            Tasks::trigger(Tasks::Pause);
        } else {
            lowBatDevices.atomicMark(id);
        }
    }
}

bool aDeviceIsLow()
{
	// was there a device with a low battery level ?
    return getNextLowBatDevice() != NONE;
}

_SYSDeviceID getNextLowBatDevice()
{
    if (selectedDevice != NONE) {
        return selectedDevice;
    }

    unsigned index;
    if (lowBatDevices.findFirst(index)) {
        setSelectedDevice(index);
        lowBatDevices.atomicClear(index);
        return uint8_t(selectedDevice);
    }

    return NONE;
}

void setSelectedDevice(_SYSDeviceID id)
{
    ASSERT(id <= BASE);

    selectedDevice = id;
    canWarn.atomicClear(id);
}

void setWarningCompleted()
{
    selectedDevice = NONE;
}

} // namespace BatteryLevel
