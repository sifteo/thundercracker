#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

void main()
{
    LOG("Starting\n");

    AudioChannel channel(0);

    channel.play(TestSound);

    while (channel.isPlaying())
        System::paint();

    LOG("Finished.\n");
}
