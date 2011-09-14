/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_SYSTEM_H
#define _SIFTEO_SYSTEM_H

#include "syscall.h"

namespace Sifteo {


/**
 * Global operations that apply to the system as a whole.
 */

class System {
 public:

    /**
     * Do nothing, until any event arrives. This allows the firmware
     * to schedule other asynchronous operations now, or to put the
     * CPU in a lower power mode.
     */

    static void yield() {
	SYS_yield();
    }

    /**
     * Draw one frame, nearly simultaneously, on every connected cube
     * that has pending graphical changes. If we're producing frames
     * faster than the cube's display refresh rate, this call will
     * block briefly to keep the game's frame rate matched with the
     * hardware frame rate.
     */

    static void draw() {
	SYS_draw();
    }
};


};  // namespace Sifteo

#endif
