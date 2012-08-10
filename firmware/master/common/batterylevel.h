#ifndef BATTERYLEVEL_H
#define BATTERYLEVEL_H

#include <stdint.h>

namespace BatteryLevel
{
    void init();
    int  currentLevel();
    void beginCapture();
    void captureIsr();
}

#endif // BATTERYLEVEL_H
