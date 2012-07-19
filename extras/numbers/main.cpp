/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static Metadata M = Metadata()
    .title("Numbers station")
    .package("com.sifteo.extras.numbers", "1.0")
    .cubeRange(0);


void sayDigit(unsigned i)
{
    static SystemTime stamp = SystemTime::now();
    static unsigned channel = 0;
    static const AssetAudio* samples[] = {
        &Sample0, &Sample1, &Sample2, &Sample3, &Sample4, &Sample5, &Sample6, &Sample7,
        &Sample8, &Sample9, &SampleA, &SampleB, &SampleC, &SampleD, &SampleE, &SampleF
    };

    i &= 0xF;

    LOG("%d\n", i);

    AudioChannel(channel).play(*samples[i]);
    channel = (channel + 1) % AudioChannel::NUM_CHANNELS;

    while (1) {
        SystemTime now = SystemTime::now();
        if (now - stamp > 0.5) {
            stamp = now;
            return;
        }
        System::yield();
    }
}

void sayUInt32(uint32_t n)
{
    for (int i = 28; i >= 0; i -= 4)
        sayDigit(n >> i);
}

void main()
{
    Random r;

    while (1)
        sayUInt32(r.raw());
}
