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

#include "ledsequencer.h"


const LEDPattern LEDPatterns::idle[] = {
    { 20,  0x00, 0x50 },  // Quick ramp up

    { 20,  0x00, 0x20 },  // Logarithmic fade down
    { 20,  0x00, 0x10 },
    { 20,  0x00, 0x08 },
    { 20,  0x00, 0x04 },
    { 20,  0x00, 0x02 },
    { 20,  0x00, 0x01 },
    { 40,  0x00, 0x00 },

    { 200, 0x00, 0x00 },  // Rest period
    { 100, 0x00, 0x00 },
    { 0 }
};

const LEDPattern LEDPatterns::panic[] = {
    { 10, 0xFF, 0x00 },
    { 10, 0x00, 0xFF },
    { 10, 0x00, 0x00 },
    { 20, 0x00, 0x00 },
    { 0 }
};

const LEDPattern LEDPatterns::paused[] = {
    { 5, 0x00, 0xFF },
    { 20, 0x00, 0x80 },
    { 0 }
};

const LEDPattern LEDPatterns::shutdownSlow[] = {
    { 5, 0xFF, 0x00 },
    { 40, 0x00, 0x00 },
    { 0 }
};

const LEDPattern LEDPatterns::shutdownFast[] = {
    { 5, 0xFF, 0x00 },
    { 20, 0x00, 0x00 },
    { 0 }
};


void LEDSequencer::tickISR(LEDState &state)
{
    if (!nextNode) {
        // Beginning a new loop after we were idle?
        nextNode = nextLoop;
    }

    if (!nextNode) {
        // No pattern is active
        tick = 0;
        prevState.red = 0;
        prevState.green = 0;
        state.red = 0;
        state.green = 0;
        return;
    }

    // Unpack the next node
    uint8_t delay = nextNode->delay;
    ASSERT(delay > 0);
    LEDState nextState = {
        expand8to16(nextNode->red),
        expand8to16(nextNode->green),
    };

    if (tick < delay) {
        // Linear interpolation
        state.red = prevState.red + ((nextState.red - prevState.red) * tick) / delay;
        state.green = prevState.green + ((nextState.green - prevState.green) * tick) / delay;
        tick++;
    } else {
        // We've reached the next node
        state = prevState = nextState;

        nextNode++;
        if (!nextNode->delay)
            nextNode = 0;
        tick = 0;
    }
}
