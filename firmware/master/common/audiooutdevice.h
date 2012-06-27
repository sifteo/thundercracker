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
typedef RingBuffer<512, int16_t> AudioBuffer;


class AudioOutDevice
{
public:
    // Start mixer with no audio device
    static void initStub() {
        Tasks::setPending(Tasks::AudioPull, NULL);
    }

    static void init(AudioMixer *mixer);
    static void start();
    static void stop();

    static void suspend();
    static void resume();
};


#endif // AUDIOOUTDEVICE_H_
