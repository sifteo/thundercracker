#include "batterylevel.h"
#include "pause.h"
#include "tasks.h"

namespace BatteryLevel {

bool needWarning()
{
    return !lowBatDevices.empty(); // was there a device with a low battery level ?
}

void setWarningDone()
{
    ASSERT(selectedCube <= BASE);
    warningDone.atomicMark(selectedCube);
    lowBatDevices.atomicClear(selectedCube);
    selectedCube = NONE;
    wasInterrupted = 0; // TODO find why bool don't work !
}

void onCapture(uint32_t batLevel, uint8_t cubeNum)
{
    // update the lowBatDevices, trigger a warning (once) if 90% discharged
    ASSERT(cubeNum <= BASE);
    if (!warningDone.test(cubeNum) && batLevel <= _SYS_BATTERY_MAX/10) {
        lowBatDevices.atomicMark(cubeNum);
        if (selectedCube == NONE || wasInterrupted!=0) { // TODO find why bool don't work !
            Pause::taskWork.atomicMark(Pause::LowBattery);
            Tasks::trigger(Tasks::Pause);
        }
    }
}

uint8_t getLowBatDevice()
{
    unsigned index = 0;
    bool needWarning = lowBatDevices.findFirst(index);
    ASSERT(needWarning); // needWarning() should be called before
    return uint8_t(index);
}

void setSelectedCube(uint8_t cubeNum)
{
    // Will tell setWarningDone() which cube should be marked as done...
    ASSERT(cubeNum <= BASE);
    selectedCube = cubeNum;
}

void setWasInterrupted()
{
    wasInterrupted = 42; // TODO find why bool don't work !
}

int getWasInterrupted() // TODO find why bool don't work !
{
    return wasInterrupted;
}

} // namespace BatteryLevel
