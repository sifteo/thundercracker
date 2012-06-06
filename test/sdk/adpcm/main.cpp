#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

void main()
{
    AudioChannel channel(0);

    channel.play(TestSound);

    while (channel.isPlaying())
        System::paint();
}