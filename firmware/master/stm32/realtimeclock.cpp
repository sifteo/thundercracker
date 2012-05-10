#include "realtimeclock.h"

void RealTimeClock::initFromOff()
{
    // Enable power and backup interface clocks
    // and access to RTC and Backup registers
    RCC.APB1ENR |= (1 << 27) | (1 << 28);
    PWR.CR | (1 << 8);

    // Turn on LSE on and wait while it stabilises.
    RCC.BDCR |= 0x1;                // LSEON
    while (!(RCC.BDCR & (1 << 1)))  // LSERDY
        ;

    // assume LSE as the clock source
    // support others eventually if we need them...
    RCC.BDCR &= ~((1 << 8) | (1 << 9));
    RCC.BDCR |= (1 << 8);

    // Enable the RTC
    RCC.BDCR |= (1 << 15);

    // Wait for Registers Synchronized Flag
    RTC.CRL &= ~(1 << 3);
    while (!(RTC.CRL & (1 << 3)))
        ;
}

void RealTimeClock::calibrate()
{
    // implement me
}
