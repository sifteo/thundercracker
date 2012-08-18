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

namespace Sifteo {


/**
 * @defgroup event Event
 *
 * @brief Event dispatch subsystem, for event-driven programming
 *
 * Event dispatch subsystem. This module allows applications to set up
 * event callbacks which are automatically invoked at system _yield points_
 * like System::paint().
 *
 * @{
 */

/**
 * @brief Implementation for a single event vector.
 *
 * Instances of this template are found in the Events namespace.
 * Typically you should not create instances of this object elsewhere.
 */

template <_SYSVectorID tID>
struct EventVector {
    EventVector() {}

    /**
     * @brief Disable this event vector.
     *
     * This acts like a no-op handler was
     * registered, but of course it's more efficient than setting an
     * actual no-op handler.
     */
    void unset() const {
        _SYS_setVector(tID, 0, 0);
    }

    /**
     * @brief Set this Vector to a function with context pointer
     *
     * Requires a closure consisting of an arbitrary
     * pointer-sized context value, and a function pointer of the form:
     *
     *   void handler(ContextType c, unsigned parameter);
     *
     * For Cube events, the parameter is the Cube ID that originated the
     * event. For Base events, the parameter has different meanings based
     * on the event type.
     */
    template <typename tContext>
    void set(void (*handler)(tContext, unsigned), tContext context) const {
        _SYS_setVector(tID, (void*) handler, reinterpret_cast<void*>(context));
    }

    /**
     * @brief Set this Vector to a bare function
     *
     * Set this event vector to a bare function which requires no context.
     * It must still take a dummy void* placeholder argument:
     *
     *   void handler(void*, unsigned parameter);
     *
     * For Cube events, the parameter is the Cube ID that originated the
     * event. For Base events, the parameter has different meanings based
     * on the event type.
     */
    void set(void (*handler)(void*, unsigned)) const {
        _SYS_setVector(tID, (void*) handler, 0);
    }

    /**
     * @brief Set this event vector to an instance method, given a class method
     * pointer and an instance of that class.
     */
    template <typename tClass>
    void set(void (tClass::*handler)(unsigned), tClass *cls) const {
        union {
            void *pVoid;
            void (tClass::*pMethod)(unsigned);
        } u;
        u.pMethod = handler;
        _SYS_setVector(tID, u.pVoid, (void*) cls);
    }

    /**
     * @brief Return the currently set handler function, as a void pointer.
     */
    void *handler() const {
        return _SYS_getVectorHandler(tID);
    }

    /**
     * @brief Return the currently set context object, as a void pointer.
     */
    void *context() const {
        return _SYS_getVectorContext(tID);
    }
};


/**
 * @brief Implementation for the `gameMenu` event vector.
 *
 * An instance of this class is found in the Events namespace.
 * Typically you should not create instances of this object elsewhere.
 */

struct GameMenuEventVector {
    GameMenuEventVector() {}

    /**
     * @brief Disable this event vector, and disable the game menu.
     *
     * The custom menu item will no longer appear on the Pause menu.
     */
    void unset() const {
        _SYS_setGameMenuLabel(0);
        _SYS_setVector(_SYS_BASE_GAME_MENU, 0, 0);
    }

    /**
     * @brief Set this Vector to a function with context pointer
     *
     * Requires a closure consisting of an arbitrary
     * pointer-sized context value, and a function pointer of the form:
     *
     *   void handler(ContextType c);
     *
     * This can set the item's label at the same time. See setLabel()
     * for details about the suggested label format.
     */
    template <typename tContext>
    void set(void (*handler)(tContext), tContext context, const char *label=0) const {
        if (label)
            _SYS_setGameMenuLabel(label);
        _SYS_setVector(_SYS_BASE_GAME_MENU, (void*) handler, reinterpret_cast<void*>(context));
    }

    /**
     * @brief Set this Vector to a bare function
     *
     * Set this event vector to a bare function which requires no context.
     * It must still take a dummy void* placeholder argument:
     *
     *   void handler(void*);
     *
     * This can set the item's label at the same time. See setLabel()
     * for details about the suggested label format.
     */
    void set(void (*handler)(void*), const char *label=0) const {
        if (label)
            _SYS_setGameMenuLabel(label);
        _SYS_setVector(_SYS_BASE_GAME_MENU, (void*) handler, 0);
    }

    /**
     * @brief Set this event vector to an instance method, given a class method
     * pointer and an instance of that class.
     *
     * This can set the item's label at the same time. See setLabel()
     * for details about the suggested label format.
     */
    template <typename tClass>
    void set(void (tClass::*handler)(), tClass *cls, const char *label=0) const {
        union {
            void *pVoid;
            void (tClass::*pMethod)();
        } u;
        u.pMethod = handler;
        if (label)
            _SYS_setGameMenuLabel(label);
        _SYS_setVector(_SYS_BASE_GAME_MENU, u.pVoid, (void*) cls);
    }

    /**
     * @brief Return the currently set handler function, as a void pointer.
     */
    void *handler() const {
        return _SYS_getVectorHandler(_SYS_BASE_GAME_MENU);
    }

    /**
     * @brief Return the currently set context object, as a void pointer.
     */
    void *context() const {
        return _SYS_getVectorContext(_SYS_BASE_GAME_MENU);
    }

    /**
     * @brief Set the label text used for the Game Menu item
     *
     * This sets the text which is displayed under the Game Menu icon on the
     * standard Pause menu. It must be between 1 and 15 characters long, and
     * we strongly recommend that it is an even number of characters, so that
     * it can be centered perfectly.
     *
     * The character set is that of the font in BG0_ROM. It is a superset of
     * ASCII, with nearly all ISO Latin-1 characters.
     */
    void setLabel(const char *label)
    {
        ASSERT(label && label[0]);
        _SYS_setGameMenuLabel(label);
    }
};


/**
 * @brief Implementation for a single neighbor event vector.
 *
 * Instances of this template are found in the Events namespace.
 * Typically you should not create instances of this object elsewhere.
 */

template <_SYSVectorID tID>
struct NeighborEventVector {
    NeighborEventVector() {}

    /**
     * @brief Disable this event vector.
     *
     * This acts like a no-op handler was
     * registered, but of course it's more efficient than setting an
     * actual no-op handler.
     */
    void unset() const {
        _SYS_setVector(tID, 0, 0);
    }

    /**
     * @brief Set this Vector to a function with context pointer
     *
     * Requires a closure consisting of an arbitrary
     * pointer-sized context value, and a function pointer of the form:
     *
     *   void handler(ContextType c, unsigned firstID, unsigned firstSide,
     *       unsigned secondID, unsigned secondSide);
     *
     * The IDs here are castable to NeighborID objects, and the Sides are
     * using the standard Side enumeration.
     */
    template <typename tContext>
    void set(void (*handler)(tContext, unsigned, unsigned, unsigned, unsigned), tContext context) const {
        _SYS_setVector(tID, (void*) handler, reinterpret_cast<void*>(context));
    }

    /**
     * @brief Set this Vector to a bare function
     *
     * Set this event vector to a bare function which requires no context.
     * It must still take a dummy void* placeholder argument:
     *
     *   void handler(void*, unsigned firstID, unsigned firstSide,
     *       unsigned secondID, unsigned secondSide);
     *
     * The IDs here are castable to NeighborID objects, and the Sides are
     * using the standard Side enumeration.
     */
    void set(void (*handler)(void*, unsigned, unsigned, unsigned, unsigned)) const {
        _SYS_setVector(tID, (void*) handler, 0);
    }

    /**
     * @brief Set this event vector to an instance method, given a class method
     * pointer and an instance of that class.
     */
    template <typename tClass>
    void set(void (tClass::*handler)(unsigned, unsigned, unsigned, unsigned), tClass *cls) const {
        union {
            void *pVoid;
            void (tClass::*pMethod)(unsigned, unsigned, unsigned, unsigned);
        } u;
        u.pMethod = handler;
        _SYS_setVector(tID, u.pVoid, (void*) cls);
    }

    /**
     * @brief Return the currently set handler function, as a void pointer.
     */
    void *handler() const {
        return _SYS_getVectorHandler(tID);
    }

    /**
     * @brief Return the currently set context object, as a void pointer.
     */
    void *context() const {
        return _SYS_getVectorContext(tID);
    }
};


/**
 * @brief Namespace of all available event vectors
 *
 * Specific system calls are defined as 'yielding', i.e. they may wait
 * until an event occurs in the runtime. System::paint() includes an
 * implicit yield, for example. System::yield() is an explicit yield.
 *
 * On any yielding system call, the system may dispatch pending event
 * handlers. These are not running in a separate thread, but they
 * can be delivered at many potential points in your program. They
 * are somewhat like Deferred Procedure Calls in Win32, or Signals in
 * UNIX-like operating systems.
 *
 * This object represents a single asynchronous event which may have
 * a handler set for it. Instances of this template exist in the
 * Events namespace.
 */

namespace Events {

    /*
     * Neighboring events
     */

    /// One neighbor connection (cube/side paired with cube/side) has been formed.
    const NeighborEventVector<_SYS_NEIGHBOR_ADD>     neighborAdd;

    /// One neighbor connection has been dissolved.
    const NeighborEventVector<_SYS_NEIGHBOR_REMOVE>  neighborRemove;

    /*
     * Cube events
     */

    /**
     * @brief A new cube has connected and is ready for use.
     *
     * This event is only sent for cubes within an application's range,
     * as defined by Metadata::cubeRange(). The current set of connected
     * cubes, retrievable with CubeSet::connected(), is updated immediately
     * prior to dispatching this event.
     */
    const EventVector<_SYS_CUBE_CONNECT>     cubeConnect;

    /**
     * @brief A formerly connected cube has been lost.
     *
     * This event is only sent if the number of cubes is still within the
     * application's range, as defined by Metadata::cubeRange(). If the
     * number of cubes falls below the application's minimum, instead of
     * generating this event the system will prompt the user to reconnect
     * a cube or to exit.
     *
     * The current set of connected cubes, retrievable with CubeSet::connected(),
     * is updated immediately prior to dispatching this event.
     */
    const EventVector<_SYS_CUBE_DISCONNECT>  cubeDisconnect;

    /// The current AssetConfiguration has finished loading on this cube.
    const EventVector<_SYS_CUBE_ASSETDONE>   cubeAssetDone;

    /// A cube's accelerometer state has changed.
    const EventVector<_SYS_CUBE_ACCELCHANGE> cubeAccelChange;

    /// A cube's touch state has changed (touch began or ended).
    const EventVector<_SYS_CUBE_TOUCH>       cubeTouch;

    /// A cube's battery level has changed measurably
    const EventVector<_SYS_CUBE_BATTERY>     cubeBatteryLevelChange;

    /**
     * @brief The application is responsible for repainting the screen on this cube
     * and checking its installed assets.
     *
     * This event is issued by the system in any case where an otherwise-invisible
     * system operation (a cube disconnecting and reconnecting, the user pausing and
     * resuming the game) has caused the contents of a cube's VRAM and/or Asset Flash
     * to require updating.
     *
     * If an application-visible cube disconnect/reconnect event has occurred, this
     * event is always delivered after the applicable cubeDisconnect and cubeConnect.
     */
    const EventVector<_SYS_CUBE_REFRESH>     cubeRefresh;

    /*
     * Base events
     */

    /// An event generated by Sifteo::AudioTracker.
    const EventVector<_SYS_BASE_TRACKER>     baseTracker;

    /// An event generated by an optional custom "game menu" item on the standard pause menu
    const GameMenuEventVector                gameMenu;

    /// A filesystem Volume was deleted
    const EventVector<_SYS_BASE_VOLUME_DELETE>  volumeDelete;

    /// A filesystem Volume was committed and is now available to read
    const EventVector<_SYS_BASE_VOLUME_COMMIT>  volumeCommit;

};  // namespace Events

/**
 * @} endgroup Event
 */

};  // namespace Sifteo
