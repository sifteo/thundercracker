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
    for (unsigned i = 0; i <= StoredObject::LIMIT; i++) {
        LOG("Writing object #%d\n", i);

        String<10> obj;
        obj << "Obj #" << i;

        StoredObject(i).write(obj);
    }
    
    LOG("Done\n");

    while (1)
        System::paint();
}
