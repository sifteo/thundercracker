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

    static void paint() {
        _SYS_paint();
    }

    /**
     * Wait for any previous paint() to finish.
     *
     * This is analogous to glFinish() in OpenGL. It doesn't enqueue
     * any new rendering, but the caller has a strong guarantee that
     * all existing rendering has completed by the time this function
     * returns.
     *
     * This function must be used if you're changing VRAM after
     * rendering a frame, and you must make absolutely sure that this
     * change doesn't affect the current frame. For example, if you're
     * making a sharp transition from one video mode to a totally
     * different one.
     *
     * Don't overuse finish(), especially not if you're concerned with
     * frame rate. Normally it's desirable to be working on building
     * the next frame while the cubes are still busy rendering the
     * previous one.
     */

    static void finish() {
        _SYS_finish();
    }

    /**
     * Fully synchronous repaint. This is just a shorthand for a
     * paint() followed by finish(). All of the performance caveats
     * that come with finish() are applicable here too.
     */

    static void paintSync() {
        _SYS_paint();
        _SYS_finish();
    }

    /**
     * Return the elapsed system time, in seconds. Guaranteed to be
     * monotonically increasing.
     */

    static float clock() {
        int64_t nanosec;
        _SYS_ticks_ns(&nanosec);
        return nanosec * 1e-9;
    }

    /**
     * Return the elapsed system time, in nanoseconds. Guaranteed to be
     * monotonically increasing.
     */

    static int64_t clockNS() {
        int64_t nanosec;
        _SYS_ticks_ns(&nanosec);
        return nanosec;
    }
};


};  // namespace Sifteo

#endif
