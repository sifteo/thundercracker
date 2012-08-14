/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _BACKLIGHT_H
#define _BACKLIGHT_H

#include <stdint.h>
#include <string.h>

#include "vtime.h"

namespace Cube {


/*
 * Any time the backlight turns on or off, this class gets that
 * edge. Since the backlight can PWM, we integrate those edges
 * into a floating point brightness value.
 */

class Backlight {
 public:

    ALWAYS_INLINE void cycle(bool state, uint64_t now)
    {
        if (state != lastState) {
            uint64_t dt = now - lastStateTimestamp;

            if (lastState)
                ticksOn += dt;
            ticksTotal += dt;

            lastState = state;
            lastStateTimestamp = now;
        }
    }

    double getBrightness()
    {
        if (ticksTotal) {
            // PWM
            double result = ticksOn / double(ticksTotal);
            ticksOn = 0;
            ticksTotal = 0;
            return result;

        } else {
            // Steady state
            return double(lastState);
        }
    }

 private:
    uint64_t lastStateTimestamp;
    uint64_t ticksOn;
    uint64_t ticksTotal;
    bool lastState;
};


};  // namespace Cube

#endif
