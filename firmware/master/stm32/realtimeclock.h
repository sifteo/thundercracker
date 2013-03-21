#ifndef REALTIMECLOCK_H
#define REALTIMECLOCK_H

#include "hardware.h"

class RealTimeClock
{
public:
    static void init();

    static uint32_t count();
    static void setCount(uint32_t v);

private:
    static void beginConfig();
    static void endConfig();
};

#endif // REALTIMECLOCK_H
