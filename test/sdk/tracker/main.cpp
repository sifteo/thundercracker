#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

void main()
{
    AudioTracker::play(TestSound);

    // Play for a fixed and deterministic duration.

    // Note that the exact duration will depend on some details of
    // siftulator's virtual-time scheduling, but we avoid paint() so that
    // it doesn't also depend on the paint controller's timing.

    while (SystemTime::now().uptimeMS() < 10000)
        System::yield();
}