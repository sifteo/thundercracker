/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_EVENT_H
#define _SIFTEO_EVENT_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>
#include <sifteo/cube.h>

namespace Sifteo {


/**
 * Parameter structure for neigbhor events.
 * This represents a pair of cube sides, which have formed or lost a connection.
 *
 * This is a POD type whose calling convention register layout matches _SYSNeighborEvent.
 */
struct NeighborEvent {
    unsigned _c0;
    unsigned _s0;
    unsigned _c1;
    unsigned _s1;

    CubeID firstCube() const { return _c0; };   /// Get the first cube in this pair
    CubeID secondCube() const { return _c1; };  /// Get the second cube in this pair

    Side firstSide() const { return Side(_s0); };   /// Get the first cube side in this pair
    Side secondSide() const { return Side(_s1); };  /// Get the second cube side in this pair
};


/**
 * Parameter structure for cube events.
 * This tells you which cube the event occurred on; other information must
 * be read from that cube's current state.
 *
 * This is a POD type whose calling convention register layout matches _SYSCubeEvent.
 */
struct CubeEvent {
    unsigned _c;

    CubeID cube() const { return _c; };         /// Explicit conversion to a CubeID
    operator CubeID() const { return _c; };     /// Implicit conversion to a CubeID
    operator unsigned() const { return _c; };   /// Implicit converstion to unsigned int
};


/**
 * Implementation for a single event vector.
 * Instances of this template are found in the Events namespace.
 * Typically you should not create instances of this object elsewhere.
 */

template <_SYSVectorID tID, typename tParam>
struct EventVector {
    EventVector() {}
    
    /**
     * Set this event vector, given a closure consisting of an arbitrary
     * pointer-sized context value, and a function pointer of the form:
     *
     *   void handler(ContextType c, ParamType id);
     */
    template <typename tContext>
    void set(void (*handler)(tContext, tParam), tContext context) const {
        _SYS_setVector(tID, (void*) handler, reinterpret_cast<void*>(context));
    }

    /**
     * Set this event vector to a bare function which requires no context.
     * It must still take a dummy void* placeholder argument:
     *
     *   void handler(void*, ParamType id);
     */
    void set(void (*handler)(void*, tParam)) const {
        _SYS_setVector(tID, (void*) handler, 0);
    }

    /**
     * Set this event vector to an instance method, given a class method
     * pointer and an instance of that class.
     */
    template <typename tClass>
    void set(void (tClass::*handler)(tParam), tClass *cls) const {
        union {
            void *pVoid;
            void (tClass::*pMethod)(tParam);
        } u;
        u.pMethod = handler;
        _SYS_setVector(tID, u.pVoid, (void*) cls);
    }

    /**
     * Return the currently set handler function, as a void pointer.
     */
    void *handler() const {
        return _SYS_getVectorHandler(tID);
    }

    /**
     * Return the currently set context object, as a void pointer.
     */
    void *context() const {
        return _SYS_getVectorContext(tID);
    }
};


/**
 * Asynchronous events.
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

    // Neighboring events
    static const EventVector<_SYS_NEIGHBOR_ADD, NeighborEvent>     neighborAdd;
    static const EventVector<_SYS_NEIGHBOR_REMOVE, NeighborEvent>  neighborRemove;

    // Cube events
    static const EventVector<_SYS_CUBE_FOUND, CubeEvent>       cubeFound;
    static const EventVector<_SYS_CUBE_LOST, CubeEvent>        cubeLost;
    static const EventVector<_SYS_CUBE_ASSETDONE, CubeEvent>   cubeAssetDone;
    static const EventVector<_SYS_CUBE_ACCELCHANGE, CubeEvent> cubeAccelChange;
    static const EventVector<_SYS_CUBE_TOUCH, CubeEvent>       cubeTouch;
    static const EventVector<_SYS_CUBE_TILT, CubeEvent>        cubeTilt;
    static const EventVector<_SYS_CUBE_SHAKE, CubeEvent>       cubeShake;

};  // namespace Events
};  // namespace Sifteo

#endif
