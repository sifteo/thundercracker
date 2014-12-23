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

#ifndef DAC_H_
#define DAC_H_

#include "hardware.h"

class Dac
{
public:
    enum BufferMode {
        BufferEnabled,
        BufferDisabled
    };

    enum Waveform {
        WaveNone,
        WaveNoise,
        WaveTriangle
    };

    enum DataFormat {
        RightAlign12Bit,
        LeftAlign12Bit,
        RightAlign8Bit
    };

    enum Trigger {
        TIM6,
        TIM3,
        TIM7,
        TIM5,
        TIM2,
        TIM4,
        Exti9,
        SwTrig,
        TrigNone
    };

    static void init();
    static void configureChannel(int ch, Trigger trig = TrigNone,
        Waveform waveform = WaveNone, uint8_t mask_amp = 0, BufferMode buffmode = BufferEnabled);

    static void enableChannel(int ch);
    static void disableChannel(int ch);

    static void enableDMA(int ch);
    static void disableDMA(int ch);

    static void writeDual(uint16_t data);
    static void write(int ch, uint16_t data, DataFormat format = RightAlign12Bit);
    static uintptr_t address(int ch, DataFormat format = RightAlign12Bit);
};

#endif // DAC_H_
