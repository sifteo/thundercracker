/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef HOMEBUTTON_H_
#define HOMEBUTTON_H_

#include <stdint.h>
#include "systime.h"

namespace HomeButton
{
    // Hardware-specific
    void init();
    bool hwIsPressed();

    // Hardware-independent
    void update();
    bool isPressed();
    bool isReleased();
    SysTime::Ticks pressDuration();
}

#endif // HOMEBUTTON_H_
