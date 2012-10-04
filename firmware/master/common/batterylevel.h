#ifndef BATTERYLEVEL_H
#define BATTERYLEVEL_H

#include <stdint.h>

namespace BatteryLevel
{
    void init();
    int  currentLevel();
    void beginCapture();
    void captureIsr();

    /*
     * Sentinel value to help determine whether a sample has successfully
     * been taken after startup.
     */
    static const int UNINITIALIZED = 0xffff;

    /*
     * Lower than this, and it's not worth starting up.
     */
    static const int STARTUP_THRESHOLD = 0x1C92;

    /*
     * Empirically measured maximum jitter in battery level readings
     */
    static const int MAX_JITTER = 0xB1;
}

#endif // BATTERYLEVEL_H
