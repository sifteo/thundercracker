/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOOUTDEVICE_H_
#define AUDIOOUTDEVICE_H_

#include <stdint.h>
#include "ringbuffer.h"
#include "tasks.h"

class AudioMixer;


class AudioOutDevice
{
public:
    static void init();
    static void start();
    static void stop();
    static int getSampleBias();     // Device-specific bias value to be added to each sample

    static const unsigned END_OF_STREAM = 0x8000;

    /*
     * On real hardware, our audio device is 100% interrupt driven, and it pulls
     * directly from the mixing buffer. On Siftulator, we don't have control over
     * how often the OS calls us back to request more audio data, and if these
     * callbacks happen to infrequently, we won't be able to extract data
     * from the AudioMixer buffer fast enough. So, for simulation only, we
     * provide a 'pull entry point which is used to give the AudioOutDevice
     * a specific place to dequeue data from the AudioMixer buffer if it wants to.
     */

    #ifdef SIFTEO_SIMULATOR
        static void pullFromMixer();
    #else
        static ALWAYS_INLINE void pullFromMixer() {}
    #endif
};


#endif // AUDIOOUTDEVICE_H_
