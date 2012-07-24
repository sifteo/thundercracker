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
};


#endif // AUDIOOUTDEVICE_H_
