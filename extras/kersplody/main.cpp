/*
 * Runtime memory fault test.
 */

#include <sifteo.h>

static Sifteo::Metadata M = Sifteo::Metadata()
    .title("Memory fault test")
    .package("com.sifteo.extras.kersplody", "1.0")
    .cubeRange(0);

void main()
{
    // Just past end of RAM
    *(volatile uint32_t*)0x18000 = 42; 
    while (1);
}
