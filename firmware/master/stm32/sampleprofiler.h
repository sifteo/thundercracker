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

#ifndef SAMPLEPROFILER_H_
#define SAMPLEPROFILER_H_

#include "hwtimer.h"
#include "usbprotocol.h"

class SampleProfiler
{
public:

    enum SubSystem {
        None,
        FlashDMA,
        AudioPull,
        SVCISR,
        RFISR,
        BluetoothISR,
    };

    enum Command {
        SetProfilingEnabled
    };

    static void init();

    static void onUSBData(const USBProtocolMsg &m);

    static void processSample(uint32_t pc);
    static void task();
    static void reportHang();

    static ALWAYS_INLINE SubSystem subsystem() {
        return subsys;
    }

    static ALWAYS_INLINE void setSubsystem(SubSystem s) {
        subsys = s;
    }

private:
    static SubSystem subsys;
    static uint32_t sampleBuf;  // currently just a single sample
    static HwTimer timer;
};

#endif // SAMPLEPROFILER_H_
