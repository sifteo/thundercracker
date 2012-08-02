/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef MC_HOMEBUTTON_H_
#define MC_HOMEBUTTON_H_

// Simulator-only interfaces
namespace HomeButton
{
    // Callable on any thread. May trigger a task.
    void setPressed(bool value);
}

#endif // MC_HOMEBUTTON_H_
