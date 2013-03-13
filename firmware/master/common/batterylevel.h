#ifndef BATTERYLEVEL_H
#define BATTERYLEVEL_H

#include <sifteo/abi.h>
#include <stdint.h>
#include "bits.h"

namespace BatteryLevel
{
    static const _SYSCubeID BASE = _SYS_NUM_CUBE_SLOTS;
    static const _SYSCubeID NONE = _SYS_NUM_CUBE_SLOTS + 1;

    void init();
    unsigned raw();
    unsigned vsys();
    unsigned scaled(_SYSCubeID cid = BASE); // master by default
    void beginCapture();
    void captureIsr();
    void process(unsigned);
    _SYSCubeID getNextLowBatDevice();
    bool aDeviceIsLow();
    void setWarningCompleted();
    void onCapture(uint32_t batLevel, _SYSCubeID cid);
    void setSelectedCube(uint8_t cid);

#ifdef SIFTEO_SIMULATOR
    void heartbeat();
    void updatePercentage(int8_t delta, _SYSCubeID cid);
    uint8_t getPercentage(_SYSCubeID cid);
#endif

    // In the following arrays, the last element refers to the master cube:
    static BitVector<_SYS_NUM_CUBE_SLOTS+1> lowBatDevices;
    static BitVector<_SYS_NUM_CUBE_SLOTS+1> canWarn;
    static _SYSCubeID selectedCube = NONE;

    /*
     * Sentinel value to help determine whether a sample has successfully
     * been taken after startup.
     */
    static const unsigned UNINITIALIZED = 0xffff;

    /*
     * Empirically measured maximum jitter in battery level readings
     */
    static const unsigned MAX_JITTER = 0x81;
}

#endif // BATTERYLEVEL_H
