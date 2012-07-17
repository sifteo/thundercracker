#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

void main()
{
    AudioTracker::play(TestSound);

    while (SystemTime::now().uptimeMS() < 5000)
        System::yield();
}
