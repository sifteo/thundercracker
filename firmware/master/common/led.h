/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _LED_H
#define _LED_H

#include <stdint.h>

namespace LED
{
    enum Color {
        OFF     = 0,
        RED     = 1,
        GREEN   = 2,
        ORANGE  = RED | GREEN,
    };

    void set(Color c);
}

#endif // _LED_H
