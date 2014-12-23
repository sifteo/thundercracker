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

#include "gpio.h"
#include "board.h"
#include "vectors.h"
#include "pulse_rx.h"

PulseRX::PulseRX(GPIOPin p) :
        pin(p)
{
}

void PulseRX::init()
{
    pin.irqInit();
    pin.irqSetFallingEdge();
    pin.irqDisable();
    pin.setControl(GPIOPin::IN_FLOAT);
}

void PulseRX::start()
{
    pulseCount = 0;
    pin.irqEnable();
}

void PulseRX::stop()
{
    pin.irqDisable();
}

uint16_t PulseRX::count()
{
    return pulseCount;
}

void PulseRX::pulseISR()
{
    // Saturate counter
    if (pulseCount < maxCount)
        pulseCount++;
}
