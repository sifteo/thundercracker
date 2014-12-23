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

#ifndef _UI_SHUTDOWN_H
#define _UI_SHUTDOWN_H

#include <sifteo/abi.h>
#include "systime.h"

class UICoordinator;


/**
 * A shutdown countdown screen, giving the user some time to
 * cancel before turning the system off.
 */

class UIShutdown {
public:
    UIShutdown(UICoordinator &uic);

    // Default main loop
    void mainLoop();

    // Entry points for integrating with an external main loop
    void init();
    void animate();

    ALWAYS_INLINE bool isDone() {
        return done;
    }

private:
    UICoordinator &uic;

    SysTime::Ticks digitTimestamp;
    uint8_t digit;
    bool done;

    void drawBackground();
    void drawLogo();
    void drawText(unsigned addr, const char *string);
    void drawDigit(unsigned number);

    void beginDigit(unsigned number);
    void resetTimestamp();

    static float easeInAndOut(float t);
};


#endif
