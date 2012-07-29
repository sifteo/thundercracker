/*
 * Flash cache thrashing test.
 * Causes flash page faults as fast as possible.
 */

#include <sifteo.h>

static Sifteo::Metadata M = Sifteo::Metadata()
    .title("Flash Cache Thrasher")
    .package("com.sifteo.extras.thrasher", "1.0")
    .cubeRange(0, CUBE_ALLOCATION);

void main()
{
    SCRIPT(LUA,
        function Filesystem:onRawRead(addr, data)
            local va = Runtime():flashToVirtAddr(addr)
            if va ~= 0 then
                print(string.format("Cache miss at %08x", va))
            end
        end

        Filesystem():setCallbacksEnabled(true)
    );

    static const uint8_t flashData[64*1024] = { 1 };
    unsigned i = 0;
    volatile uint32_t dummy;

    while (1) {
        volatile uint32_t* ptr = (volatile uint32_t*)(flashData + i);
        LOG("Reading %p\n", ptr);
        dummy = *ptr;
        i = (i + 256) % sizeof flashData;
    }
}
