#include "batterylevel.h"
#include "pause.h"
#include "tasks.h"

namespace BatteryLevel {

bool needWarning()
{
    return getLowBatDevice() != NONE; // was there a device with a low battery level ?
}

void setWarningDone()
{
    selectedCube = NONE;
}

void onCapture(uint32_t batLevel, uint8_t cid)
{
    if (Pause::busy)
        return;

    // trigger a warning (once) if 90% discharged
    ASSERT(cid <= BASE);
    if (!warningDone.test(cid) && batLevel <= _SYS_BATTERY_MAX/10) {
        if (selectedCube == NONE) {
            setSelectedCube(cid);
            Pause::taskWork.atomicMark(Pause::LowBattery);
            Tasks::trigger(Tasks::Pause);
        } else {
            lowBatDevices.atomicMark(cid);
        }
    }
}

uint8_t getLowBatDevice()
{
    if (selectedCube != NONE || Pause::busy) {
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

void setSelectedCube(uint8_t cid)
{
    if (Pause::busy)
        return;

    ASSERT(cid <= BASE);
    selectedCube = cid;
    warningDone.atomicMark(cid);
}

} // namespace BatteryLevel
