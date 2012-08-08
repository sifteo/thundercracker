#ifndef BATTERYLEVEL_H
#define BATTERYLEVEL_H

#include <stdint.h>

namespace BatteryLevel
{
    void init();
    int  currentLevel();
    void heartbeat();
    void captureIsr(uint16_t rawvalue);
}

#endif // BATTERYLEVEL_H
