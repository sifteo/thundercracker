/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static Metadata M = Metadata()
    .title("Numbers station")
    .package("com.sifteo.extras.numbers", "1.0")
    .cubeRange(0);


class Talker
{
    SystemTime stamp;

public:
    Talker() : stamp(SystemTime::now()) {}

    void pause(TimeDelta duration) {
        stamp += duration;
    }

    void sayDigit(unsigned i)
    {
        while (SystemTime::now() < stamp)
            System::yield();
        
        static unsigned channel = 0;
        static const AssetAudio* samples[] = {
            &Sample0, &Sample1, &Sample2, &Sample3, &Sample4, &Sample5, &Sample6, &Sample7,
            &Sample8, &Sample9, &SampleA, &SampleB, &SampleC, &SampleD, &SampleE, &SampleF
        };

        AudioChannel(channel).play(*samples[i & 0xF]);
        channel = (channel + 1) % AudioChannel::NUM_CHANNELS;
    }

    void sayUInt32(uint32_t n)
    {
        for (int i = 28; i >= 0; i -= 4) {
            sayDigit(n >> i);
            pause(0.4);
        }

        pause(1.0);
    };
};


void main()
{
    Random r;
    Talker t;

    while (1) {
        t.sayUInt32(r.raw());
    }
}
