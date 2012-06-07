/*
 * Just some random LFS testing...
 * The useful parts of this, if any, will eventually end up in test/sdk/filesystem.
 */

#include <sifteo.h>
using namespace Sifteo;

static Metadata M = Metadata()
    .title("LFS Muncher")
    .cubeRange(0);

void main()
{
    uint32_t counter = 0x1234;
    StoredObject store(10);

    while (1) {
        if (!store.read(counter))
            counter = 0;

        counter++;

        store.write(counter);
        LOG_INT(counter);
    }
}
