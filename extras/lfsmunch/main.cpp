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
    for (unsigned i = 0; i < 10; i++) {
        LOG("Writing object #%d\n", i);

        String<15> obj;
        obj << "Obj #" << i;

        if (StoredObject(i % StoredObject::LIMIT).write(obj) <= 0)
            break;
    }
    
    LOG("Done writing, trying to read\n");

    {
        String<15> obj;
        if (StoredObject(0).read(obj)) {
            LOG("Found object: '%s'\n", obj.c_str());
        }
    }

    while (1)
        System::paint();
}
