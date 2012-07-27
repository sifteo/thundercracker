/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "mc_led.h"

namespace LED {
    Color currentColor;
}

void LED::set(Color c)
{
    /*
     * Currently this can be trivial. There are no event-driven
     * consumers of Color, just the graphical frontend which
     * polls it at each frame.
     */

    currentColor = c;
}
