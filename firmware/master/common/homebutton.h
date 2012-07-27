/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef HOMEBUTTON_H_
#define HOMEBUTTON_H_

namespace HomeButton
{
    // Hardware-specific
    void init();
    bool isPressed();

    // Hardware-independent
    void task();
    void shutdown();
}

#endif // HOMEBUTTON_H_
