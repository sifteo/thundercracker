/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>

namespace Sifteo {


/**
 * Implementation for a single per-cube event vector.
 *
 * Instances of this template are found in the Events namespace.
 * Typically you should not create instances of this object elsewhere.
 */

template <_SYSVectorID tID>
struct CubeEventVector {
    CubeEventVector() {}

    /**
     * Set this event vector, given a closure consisting of an arbitrary
     * pointer-sized context value, and a function pointer of the form:
     *
     *   void handler(ContextType c, unsigned cubeID);
     */
    template <typename tContext>
    void set(void (*handler)(tContext, unsigned), tContext context) const {
        _SYS_setVector(tID, (void*) handler, reinterpret_cast<void*>(context));
    }

    /**
     * Set this event vector to a bare function which requires no context.
     * It must still take a dummy void* placeholder argument:
     *
     *   void handler(void*, unsigned cubeID);
     */
    void set(void (*handler)(void*, unsigned)) const {
        _SYS_setVector(tID, (void*) handler, 0);
    }

    /**
     * Set this event vector to an instance method, given a class method
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
 * Implementation for a single neighbor event vector.
 *
 * Instances of this template are found in the Events namespace.
 * Typically you should not create instances of this object elsewhere.
 */

template <_SYSVectorID tID>
struct NeighborEventVector {
    NeighborEventVector() {}

    /**
     * Set this event vector, given a closure consisting of an arbitrary
     * pointer-sized context value, and a function pointer of the form:
     *
     *   void handler(ContextType c, unsigned firstCube, unsigned firstSide,
     *       unsigned secondCube, unsigned secondSide);
     */
    template <typename tContext>
    void set(void (*handler)(tContext, unsigned, unsigned, unsigned, unsigned), tContext context) const {
        _SYS_setVector(tID, (void*) handler, reinterpret_cast<void*>(context));
    }

    /**
     * Set this event vector to a bare function which requires no context.
     * It must still take a dummy void* placeholder argument:
     *
     *   void handler(void*, unsigned firstCube, unsigned firstSide,
     *       unsigned secondCube, unsigned secondSide);
     */
    void set(void (*handler)(void*, unsigned, unsigned, unsigned, unsigned)) const {
        _SYS_setVector(tID, (void*) handler, 0);
    }

    /**
     * Set this event vector to an instance method, given a class method
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
    static const NeighborEventVector<_SYS_NEIGHBOR_ADD>     neighborAdd;
    static const NeighborEventVector<_SYS_NEIGHBOR_REMOVE>  neighborRemove;

    // Cube events
    static const CubeEventVector<_SYS_CUBE_FOUND>       cubeFound;
    static const CubeEventVector<_SYS_CUBE_LOST>        cubeLost;
    static const CubeEventVector<_SYS_CUBE_ASSETDONE>   cubeAssetDone;
    static const CubeEventVector<_SYS_CUBE_ACCELCHANGE> cubeAccelChange;
    static const CubeEventVector<_SYS_CUBE_TOUCH>       cubeTouch;
    static const CubeEventVector<_SYS_CUBE_TILT>        cubeTilt;
    static const CubeEventVector<_SYS_CUBE_SHAKE>       cubeShake;

};  // namespace Events
};  // namespace Sifteo
