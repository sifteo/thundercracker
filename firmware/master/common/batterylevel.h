#ifndef BATTERYLEVEL_H
#define BATTERYLEVEL_H

#include <sifteo/abi.h>
#include <stdint.h>
#include "bits.h"
#include "batterymonitor.h"

namespace BatteryLevel
{
    void init();
    unsigned raw();
    unsigned vsys();
    unsigned scaled(_SYSCubeID cid = BatteryMonitor::BASE);
    void beginCapture();
    void captureIsr();
    void process(unsigned);

#ifdef SIFTEO_SIMULATOR
    void heartbeat();
    void updatePercentage(int8_t delta);
    uint8_t getPercentage();
#endif

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
