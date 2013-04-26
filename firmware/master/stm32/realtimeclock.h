#ifndef REALTIMECLOCK_H
#define REALTIMECLOCK_H

#include "hardware.h"
#include "macros.h"

class RealTimeClock
{
public:
    static void beginInit();
    static void finishInit();

    static ALWAYS_INLINE uint32_t count() {
        return (RTC.CNTH << 16) | RTC.CNTL;
    }
    static void setCount(uint32_t v);

private:

    static ALWAYS_INLINE void beginConfig() {
        // ensure any previous writes have completed
        while ((RTC.CRL & (1 << 5)) == 0) {
            ;
        }
        RTC.CRL |= (1 << 4);    // CNF: enter configuration mode
    }

    static ALWAYS_INLINE void endConfig() {
        RTC.CRL &= ~(1 << 4);   // Exit configuration mode
    }
};

#endif // REALTIMECLOCK_H
