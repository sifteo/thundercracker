/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _LED_H
#define _LED_H

/*
 * Hardware-independent LED abstraction.
 *
 * The hardware-specific implementation hooks into LEDSequencer,
 * giving us a way to play back simple animations.
 */

#include "ledsequencer.h"

namespace LED
{
    void init();
    void set(const LEDPattern *pattern, bool immediate=false);
}

#endif // _LED_H
