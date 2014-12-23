/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
