#ifndef BATTERYLEVEL_H
#define BATTERYLEVEL_H

#include <stdint.h>

namespace BatteryLevel
{
    void init();
    int  raw();
    int  scaled();
    void beginCapture();
    void captureIsr();

    /*
     * Sentinel value to help determine whether a sample has successfully
     * been taken after startup.
     */
    static const unsigned UNINITIALIZED = 0xffff;

    static const unsigned STARTUP_THRESHOLD = 0x1C30;

    /*
     * Empirically measured maximum jitter in battery level readings
     */
    static const unsigned MAX_JITTER = 0xB1;
}

#endif // BATTERYLEVEL_H
