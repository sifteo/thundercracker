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

#ifndef _LED_SEQUENCER_H
#define _LED_SEQUENCER_H

#include <stdint.h>
#include <string.h>
#include "macros.h"

/**
 * An array of LEDPattern nodes can describe an animated sequence
 * of LED brightness changes. It is described as a sequence of 
 * timestamped "keyframes" which we linearly interpolate between.
 *
 * Each entry in the LEDPattern array contains a delay, in clock ticks,
 * since the last pattern node. The first node's delay is assumed to be
 * relative to the end of the last loop. The list is terminated
 * by a pattern entry with delay=0.
 */

struct LEDPattern {
    uint8_t delay;
    uint8_t red;
    uint8_t green;
};

/**
 * Canned LED patterns we can choose from
 */

namespace LEDPatterns {
    extern const LEDPattern idle[];
    extern const LEDPattern panic[];
    extern const LEDPattern paused[];
    extern const LEDPattern shutdownSlow[];
    extern const LEDPattern shutdownFast[];
}

/**
 * The LEDSequencer plays back an animation sequence from
 * an LEDPattern, in an event-driven way. We accept a hardware
 * specific ISR callback which advances the state of the sequencer
 * by one tick and emits new PWM values for each LED.
 */

class LEDSequencer {
public:
    // Frame rate for animation, and timebase for pattern "ticks"
    static const unsigned TICK_HZ = 100;
    
    struct LEDState {
        // 16-bit fixed point
        uint16_t red, green;
    };

    void init() {
        memset(this, 0, sizeof *this);
    }

    /*
     * Normally takes effect after the end of the current loop,
     * unless 'immediate' is true and the specified pattern isn't
     * already current.
     *
     * If pattern is NULL, the LED will go dark after the end of the loop.
     */
    void setPattern(const LEDPattern *pattern, bool immediate=false) {
        if (immediate && nextLoop != pattern) {
            // Racy; not guaranteed to work. Worst case, we act like immediate==false.
            nextNode = pattern;
        }
        nextLoop = pattern;
    }

    void tickISR(LEDState &state);

private:
    // Write outside ISR, read inside ISR
    const LEDPattern *nextLoop;

    // Normally accessed by ISR only, but may be smashed by setPattern()
    const LEDPattern *nextNode;

    // Accessed by ISR only
    LEDState prevState;
    uint8_t tick;

    // Expand 8-bit to 16-bit color
    static ALWAYS_INLINE uint16_t expand8to16(uint8_t x) {
        return (uint16_t(x) << 8) | x;
    }
};

#endif // _LED_SEQUENCER_H
