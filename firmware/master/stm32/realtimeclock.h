#ifndef REALTIMECLOCK_H
#define REALTIMECLOCK_H

#include "hardware.h"

class RealTimeClock
{
public:
    static void initFromOff();

    static uint32_t count() {
        return ((RTC.CNTH << 16) | RTC.CNTL);
    }

private:
    static void calibrate();
};

#endif // REALTIMECLOCK_H
