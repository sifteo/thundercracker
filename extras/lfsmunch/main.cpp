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
    for (unsigned i = 0;; i++) {
        LOG("Writing object #%d\n", i);

        String<199> obj;
        obj << "Obj #" << i;

        if (StoredObject(i % StoredObject::LIMIT).write(obj) <= 0)
            break;
    }
    
    LOG("Done\n");

    while (1)
        System::paint();
}
