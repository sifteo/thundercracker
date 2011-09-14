/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_SYSTEM_H
#define _SIFTEO_SYSTEM_H

#include "abi.h"

namespace Sifteo {

/**
 * Global operations that apply to the system as a whole.
 */

class System {
 public:
    
    /**
     * Leave the game immediately, returning control back to the main
     * menu.  Equivalent to returning from siftmain().
     */

    static void exit() {
	_SYS_exit();
    }

    /**
     * Temporarily give up control of the CPU. During a yield(), event
     * callbacks may run. If the system as a whole has nothing better to
     * do, the CPU will be put into a lower-power mode until some kind
     * of event occurs.
     *
     * yield() may return at any time, including immediately after
     * it's called. There is no guarantee that any particular event
     * will have occurred before it returns.
     */

    static void yield() {
	_SYS_yield();
    }

    /**
     * Draw the next frame, simultaneously on all enabled and connected cubes.
     *
     * This function includes flow control. If the game is rendering
     * faster than the cubes are, this function automatically yields
     * to keep the game's frame rate in sync with the cube frame rate.
     *
     * This function can very cheaply determine whether any changes in
     * fact occurred on a cube. If nothing changed, we won't redraw
     * that cube.  If no cubes need drawing, this function yields for
     * the duration of one frame.
     */

    static void draw() {
	_SYS_draw();
    }

};


};  // namespace Sifteo

#endif
