/*
 * Flash cache thrashing test.
 * Causes flash page faults as fast as possible.
 */

#include <sifteo.h>

void main()
{
    Sifteo::Metadata().title("Flash Cache Thrasher");

    static const uint8_t flashData[64*1024] = { 1 };
    unsigned i = 0;
    volatile uint32_t dummy;

    while (1) {
        dummy = *(volatile uint32_t*)(flashData + i);
        i = (i + 256) % sizeof flashData;
    }
}
