#ifndef BATTERYLEVEL_H
#define BATTERYLEVEL_H

#include <stdint.h>

namespace BatteryLevel
{
    void init();
    unsigned raw();
    unsigned vsys();
    unsigned scaled();
    void beginCapture();
    void captureIsr();

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
