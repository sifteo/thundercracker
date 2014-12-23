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

#include "dac.h"

void Dac::init()
{
    RCC.APB1ENR |= (1 << 29); // enable dac peripheral clock
}

// channels: 1 or 2
void Dac::configureChannel(int ch, Trigger trig, Waveform waveform, uint8_t mask_amp, BufferMode buffmode)
{
    uint16_t reg =  (mask_amp << 8) |
                    (waveform << 6) |
                    (trig == TrigNone ? 0 : (trig << 3) | (1 << 2)) | // if not none, both enable the trigger and configure it
                    (buffmode << 1);
    DAC.CR |= (reg << ((ch - 1) * 16));
}

void Dac::enableChannel(int ch)
{
    DAC.CR |= (1 << ((ch - 1) * 16));
}

void Dac::disableChannel(int ch)
{
    DAC.CR &= ~(1 << ((ch - 1) * 16));
}

void Dac::enableDMA(int ch)
{
    DAC.CR |= (0x1000 << ((ch - 1) * 16));
}

void Dac::disableDMA(int ch)
{
    DAC.CR &= ~(0x1000 << ((ch - 1) * 16));
}    

uintptr_t Dac::address(int ch, DataFormat format)
{
    volatile DACChannel_t &dc = DAC.channels[ch - 1];
    return (uintptr_t) &dc.DHR[format];
}

void Dac::write(int ch, uint16_t data, DataFormat format)
{
    volatile DACChannel_t &dc = DAC.channels[ch - 1];
    dc.DHR[format] = data;
}

// TODO - this is only 8-bit dual, support other formats/alignments as needed
void Dac::writeDual(uint16_t data)
{
    DAC.DHR8RD = data;
}

