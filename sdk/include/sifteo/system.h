/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>
#include <sifteo/macros.h>

namespace Sifteo {

/**
 * @defgroup system System
 *
 * @brief Global operations that apply to the system as a whole.
 *
 * @{
 */

/**
 * @brief Global operations that apply to the system as a whole.
 */

class System {
 public:
    
    /**
     * @brief Leave the game immediately
     *
     * Returns control back to the main menu. Equivalent to returning
     * from main().
     */

    static void exit() {
        _SYS_exit();
    }

    /**
     * @brief Exit with a fault, for fatal error handling.
     *
     * This is the same kind of fatal error that occurs when an ASSERT() fails.
     */

    static void abort() {
        _SYS_abort();
    }

    /**
     * @brief Temporarily give up control of the CPU.
     *
     * During a yield(), event callbacks may run. If the system as a whole
     * has nothing better to do, the CPU will be put into a lower-power mode
     * until some kind of event occurs.
     *
     * yield() may return at any time, including immediately after
     * it's called. There is no guarantee that any particular event
     * will have occurred before it returns.
     *
     * Handlers registered with Sifteo::Events may be dispatched immediately
     * prior to returning from yield().
     */

    static void yield() {
        _SYS_yield();
    }

    /**
     * @brief Draw the next frame
     *
     * Draws simultaneously on all enabled and connected cubes.
     *
     * This function includes flow control. If the game is rendering
     * faster than the cubes are, this function automatically yields
     * to keep the game's frame rate in sync with the cube frame rate.
     *
     * This function can very cheaply determine whether any changes in
     * fact occurred on a cube. If nothing changed, we won't redraw
     * that cube.  If no cubes need drawing, this function yields for
     * the duration of one frame.
     *
     * Handlers registered with Sifteo::Events may be dispatched immediately
     * prior to returning from paint().
     */

    static void paint() {
        _SYS_paint();
    }

    /**
     * @brief Draw the next frame, without an upper limit on frame rate.
     *
     * Normally paint() includes various forms of throttling which prevent
     * the game from rendering faster than the LCD can display frames,
     * or faster than the cube can keep up. Because of this, a normal paint()
     * includes two phases: waiting for time to paint, then triggering a
     * paint on all cubes.
     *
     * This variant of paint() skips the former step. We immediately trigger
     * a render on all cubes, without regard to putting any explicit upper
     * limits on the rate at which we're completing frames.
     *
     * Handlers registered with Sifteo::Events may be dispatched immediately
     * prior to returning from paintUnlimited().
     */

    static void paintUnlimited() {
        _SYS_paintUnlimited();
    }

    /**
     * @brief Wait for any previous paint() to finish.
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
     * Typically you should call finish() *before* an event that changes
     * the video mode or the BG1 mask.
     *
     * Don't overuse finish(), especially not if you're concerned with
     * frame rate. Normally it's desirable to be working on building
     * the next frame while the cubes are still busy rendering the
     * previous one.
     *
     * Unlike paint() and yield(), finish does *not* dispatch Sifteo::Events
     * handler functions.
     */

    static void finish() {
        _SYS_finish();
    }

    /**
     * @brief Is this a debug build?
     *
     * Returns 'true' if slinky was invoked with '-g', or false otherwise.
     */
    static bool isDebug() {
        return _SYS_lti_isDebug();
    }

    /**
     * @brief Is this a debug build running in simulation?
     *
     * Returns 'true' if slinky was invoked with '-g' and we're running in
     * siftulator rather than on real hardware. Returns false otherwise.
     *
     * Release builds cannot detect being run in Siftulator in this manner.
     * On release buids, this function always returns false.
     */
    static bool isSimDebug()
    {
        unsigned flag = 0;
        SCRIPT_FMT(LUA, "Runtime():poke(%p, 1)", &flag);
        return flag;
    }

    /**
     * @brief Get the system's battery level.
     *
     * The battery level has already been converted into a "fuel gauge"
     * representation, with 0.0f representing a qualitatively dead
     * battery and 1.0f representing a totally new battery.
     *
     * A reading of exactly zero will only result when the battery level is not 
     * yet known. This is only relevant to code which can run very soon after
     * the system boots.
     *
     * To get the battery level for a single cube, call CubeID::batteryLevel().
     */
    static float batteryLevel() {
        return _SYS_sysBatteryLevel() / float(_SYS_BATTERY_MAX);
    }

    /**
     * @brief Shut down the system.
     *
     * This initiates a shutdown sequence. By default, this will pause the game and
     * present the system's standard shutdown UI. If shutdown is cancelled, this
     * function returns. Otherwise, the base shuts down immediately and this function
     * will not return.
     *
     * Games may elect to use this to shut down during periods of inactivity, such as if
     * the user leaves a menu idle for a while.
     *
     * If the 'now' flag is true, the UI is bypassed and we shut down immediately.
     */
    static void shutdown(bool now = false) {
        return _SYS_shutdown(now ? 0 : _SYS_SHUTDOWN_WITH_UI);
    }

    /**
     * @brief Prevent the system from shutting down due to inactivity.
     *
     * By default, the system shuts down automatically after 10 minutes
     * of inactivity - no touch or neighbor events. If an app wants to keep
     * running, and does not expect to generate any neighbor or touch events
     * during normal operation, keepAwake() resets the timeout countdown.
     */
    static void keepAwake() {
        _SYS_keepAwake();
    }

    /**
     * @brief Set the range of cubes supported.
     *
     * When the number of cubes connected to the system drops below `minimum`,
     * the system pauses execution until the requisite number of cubes are
     * connected, or the user decides to quit.
     *
     * When the number of cubes connected to the system is greater than
     * `maximum`, extra cubes will not be reported as connected to userspace. NOTE: the
     * behavior for cubes in this limbo state has not been fully specified.
     *
     * In many cases, it is not required to call setCubeRange() explicitly -
     * including a call to Metadata::cubeRange() is sufficient to set the
     * default cube range for the app. During execution, setCubeRange()
     * can be used to dynamically configure the number of cubes.
     *
     * @note Calling setCubeRange() when the number of cubes connected to the
     * system is below the new `minimum` will trigger a pause immediately
     * until the required cubes are connected.
     */
    static void setCubeRange(unsigned minimum, unsigned maximum) {
        _SYS_setCubeRange(minimum, maximum);
    }

    /**
     * @brief Helper to specify a cube range with identical minimum and maximum counts.
     */
    static void setCubeRange(unsigned count) {
        _SYS_setCubeRange(count, count);
    }
};

/**
 * @} end defgroup system
 */

};  // namespace Sifteo
