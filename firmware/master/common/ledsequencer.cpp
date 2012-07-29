/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "ledsequencer.h"


const LEDPattern LEDPatterns::idle[] = {
    { 30, 0x00, 0xFF },
    { 70, 0x00, 0x00 },
    { 200, 0x00, 0x00 },
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

const LEDPattern LEDPatterns::shutdown[] = {
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
