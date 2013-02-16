#ifndef BATTERYLEVEL_H
#define BATTERYLEVEL_H

#include <sifteo/abi.h>
#include <stdint.h>
#include "bits.h"

namespace BatteryLevel
{
    static const unsigned BASE = _SYS_NUM_CUBE_SLOTS;

    void init();
    unsigned raw();
    unsigned vsys();
    unsigned scaled(unsigned cubeNum = BASE); // master by default
    void beginCapture();
    void captureIsr();
    void process(unsigned);
    void heartbeat();
    unsigned getWeakCube();
    bool needWarning();
    void setWarningDone(unsigned cubeNum);
    void onCapture();
    void updatePercentage(int delta, unsigned cubeNum);
    unsigned getPercentage(unsigned cubeNum);

    // In the following arrays, the last element refers to the master cube:
    static uint8_t percentage[_SYS_NUM_CUBE_SLOTS+1] = {0}; // will be 100% in init()
    static bool warningDone[_SYS_NUM_CUBE_SLOTS+1] = {0};
    static BitVector<_SYS_NUM_CUBE_SLOTS+1> needWarningVect;

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
