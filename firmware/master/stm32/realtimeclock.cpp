#include "realtimeclock.h"
#include "macros.h"

void RealTimeClock::init()
{
    // Enable power and backup interface clocks
    // and access to RTC and Backup registers
    RCC.APB1ENR |= (1 << 27) | (1 << 28);
    PWR.CR |= (1 << 8); // DBP: Disable backup domain write protection

    // assume LSE as the clock source
    // support others eventually if we need them...

    // are we waking up from standby or power down?
    if ((RCC.BDCR & (1 << 15)) == 0) {

        RCC.BDCR |= (1 << 16);  // BDRST - backup domain reset
        RCC.BDCR &= ~(1 << 16);

        // turn on LSE on and wait while it stabilises.
        RCC.BDCR |= (1 << 0);                   // LSEON
        while ((RCC.BDCR & (1 << 1)) == 0) {    // LSERDY
            ;
        }

        // select LSE as the source
        RCC.BDCR &= ~((1 << 8) | (1 << 9));
        RCC.BDCR |= (1 << 8);

        // Enable the RTC
        RCC.BDCR |= (1 << 15);
    }

    // wait for Registers Synchronized Flag
    RTC.CRL &= ~(1 << 3);
    while (!(RTC.CRL & (1 << 3)))
        ;
}

ALWAYS_INLINE uint32_t RealTimeClock::count()
{
    /*
     * We don't make any claims as to the semantics of this value,
     * we just treat it as a count of seconds.
     */

    return ((RTC.CNTH << 16) | RTC.CNTL);
}

void RealTimeClock::setCount(uint32_t v)
{
    beginConfig();

    RTC.CNTH = (v & 0xffff0000) >> 16; // CNT[31:16]
    RTC.CNTL = v & 0x0000ffff;         // CNT[15:0]

    endConfig();
}

ALWAYS_INLINE void RealTimeClock::beginConfig()
{
    // ensure any previous writes have completed
    while ((RTC.CRL & (1 << 5)) == 0) {
        ;
    }

    RTC.CRL |= (1 << 4);    // CNF: enter configuration mode
}

ALWAYS_INLINE void RealTimeClock::endConfig()
{
    RTC.CRL &= ~(1 << 4);   // Exit configuration mode
}
