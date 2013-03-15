#ifndef BATTERYMONITOR_H
#define BATTERYMONITOR_H

#include <sifteo/abi.h>
#include "bits.h"

namespace BatteryMonitor
{
    static const _SYSDeviceID BASE = _SYS_NUM_CUBE_SLOTS;
    static const _SYSDeviceID NONE = _SYS_NUM_CUBE_SLOTS + 1;

    void init();
    void onCapture(uint32_t batLevel, _SYSDeviceID id);
    bool aDeviceIsLow();
    _SYSDeviceID getNextLowBatDevice();
    void setSelectedDevice(_SYSDeviceID id);
    void setWarningCompleted();

    // In the following arrays, the last element refers to the master cube:
    static BitVector<_SYS_NUM_CUBE_SLOTS+1> lowBatDevices;
    static BitVector<_SYS_NUM_CUBE_SLOTS+1> canWarn;
    static _SYSDeviceID selectedDevice = NONE;
}

#endif // BATTERYMONITOR_H
