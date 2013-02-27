#include "batterylevel.h"
#include "pause.h"
#include "tasks.h"
#include "cubeslots.h"

namespace BatteryLevel {

bool needWarning()
{
    return !needWarningVect.empty(); // is there a flag set ?
}

void setWarningDone(unsigned cubeNum)
{
    warningDone[cubeNum] = true;
    needWarningVect.clear(cubeNum);
}

void onCapture() // warn the user (once) when reach 90% discharge
{
    if (!warningDone[BASE] && _SYS_sysBatteryLevel() <= _SYS_BATTERY_MAX/10) {
        needWarningVect.atomicMark(BASE);
    }
    for (_SYSCubeID i = 0; i < CubeSlots::maxUserCubes; i++) {
        if (!warningDone[i] && _SYS_cubeBatteryLevel(i) <= _SYS_BATTERY_MAX/10) {
            needWarningVect.atomicMark(i);
        }
    }
    if (!needWarningVect.empty()) {
        Pause::taskWork.atomicMark(Pause::LowBattery);
        Tasks::trigger(Tasks::Pause);
    }
}

unsigned getWeakCube()
{
    unsigned cubeNum;
    needWarningVect.findFirst(cubeNum); // update the cubeNum index by reference
    return cubeNum;
}

} // namespace BatteryLevel
