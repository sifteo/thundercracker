#ifndef BATTERYMONITOR_H
#define BATTERYMONITOR_H

#include <sifteo/abi.h>
#include <stdint.h>
#include "bits.h"

namespace BatteryMonitor
{
    static const _SYSCubeID BASE = _SYS_NUM_CUBE_SLOTS;
    static const _SYSCubeID NONE = _SYS_NUM_CUBE_SLOTS + 1;

    void init();
    void onCapture(uint32_t batLevel, _SYSCubeID cid);
    bool aDeviceIsLow();
    _SYSCubeID getNextLowBatDevice();
    void setSelectedCube(_SYSCubeID cid);
    void setWarningCompleted();

    // In the following arrays, the last element refers to the master cube:
    static BitVector<_SYS_NUM_CUBE_SLOTS+1> lowBatDevices;
    static BitVector<_SYS_NUM_CUBE_SLOTS+1> canWarn;
    static _SYSCubeID selectedCube = NONE;
}

#endif // BATTERYMONITOR_H
