/*
 * Just some random LFS testing...
 * The useful parts of this, if any, will eventually end up in test/sdk/filesystem.
 */

#include <sifteo.h>
using namespace Sifteo;

static Metadata M = Metadata()
    .title("LFS Muncher")
    .package("com.sifteo.extras.lfsmunch", "1.0")
    .cubeRange(0);

void main()
{
    SCRIPT(LUA,
        function Filesystem:onRawRead(addr, data)
            print(string.format("Read at  %08x, %d bytes", addr, string.len(data)))
        end

        function Filesystem:onRawWrite(addr, data)
            print(string.format("Write at %08x, %d bytes", addr, string.len(data)))
        end

        function Filesystem:onErase(addr)
            print(string.format("Erase at %08x", addr))
        end

        Filesystem():setCallbacksEnabled(true)
    );

    uint32_t counter = 0x1234;
    StoredObject store(10);

    while (1) {
        if (store.read(counter) <= 0)
            counter = 0;

        counter++;

        store.write(counter);
        LOG_INT(counter);
    }
}
