/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef HOMEBUTTON_H_
#define HOMEBUTTON_H_

#include <stdint.h>
#include "systime.h"

extern SysTime::Ticks sampleTicks[100];
extern uint8_t sampleTicksPointer;

namespace HomeButton
{
    // Hardware-specific
    void init();
    bool isPressed();
    ALWAYS_INLINE bool isReleased() {
        return !isPressed();
    }

    // Hardware-independent
    void update();
    SysTime::Ticks pressDuration();
}

#endif // HOMEBUTTON_H_
