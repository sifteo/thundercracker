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
     * Value of low battery startup threshold
     */
    static const int STARTUP_THRESHOLD = 0x1C92;
    static const int THRESHOLD_DIFF = 0x00B1;
}

#endif // BATTERYLEVEL_H
