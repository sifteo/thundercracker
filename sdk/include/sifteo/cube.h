/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_CUBE_H
#define _SIFTEO_CUBE_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/macros.h>
#include <sifteo/math.h>

namespace Sifteo {


/**
 * Side is an enumeration which names the four sides of a Sifteo cube.
 * Depending on context, these names can refer to either "physical" sides
 * (relative to the hardware in a cube) or "virtual" sides (relative to the
 * current screen rotation.)
 */

enum Side {
    TOP = 0,
    LEFT,
    BOTTOM,
    RIGHT,
    NUM_SIDES,
    NO_SIDE = -1
};


/**
 * CubeID is a lightweight identifier for one Sifteo cube. It is a small
 * unsigned integer which refers to a "cube slot", a collection of resources
 * that the operating system maintains for each cube or potential cube.
 *
 * CubeID objects can be used as accessors for state that is stored in this
 * "cube slot", and as mutators to change the state of the slot. Use a CubeID
 * to read sensor state from a cube, to connect a new cube, or as a parameter
 * for other APIs which need to tell cubes apart.
 */

struct CubeID {
    _SYSCubeID sys;

    /// The maximum number of distinct CubeIDs.
    static const _SYSCubeID NUM_SLOTS = _SYS_NUM_CUBE_SLOTS;

    /// A reserved ID, used to mark undefined CubeIDs.
    static const _SYSCubeID UNDEFINED = _SYS_CUBE_ID_INVALID;

    /**
     * Default constructor. By default, a CubeID is initialized to a
     * special Undefined value. This value can be tested for with the
     * isDefined() predicate.
     */
    CubeID() : sys(UNDEFINED) {}

    /**
     * Initialize a CubeID with a concrete slot value. Slots are numbered
     * from 0 to NUM_SLOTS - 1.
     */
    CubeID(_SYSCubeID sys) : sys(sys) {}

    /**
     * Implicit conversion to _SYSCubeID, for use in low-level system calls.
     */
    operator _SYSCubeID() const {
        return sys;
    }

    /**
     * Return the _SYSCubeIDVector bit associated with this ID. These bits
     * are used for some system calls, and they can be used to quickly
     * describe a set of cubes.
     */
     
    _SYSCubeIDVector bit() const {
        return 0x80000000 >> sys;
    }
    
    /**
     * Is this a CubeID that was initialized with a valid slot number, and
     * not one that was initialized as undefined?
     */
    bool isDefined() const {
        return sys != UNDEFINED;
    }

    /**
     * Return the physical accelerometer state, as a signed byte-vector.
     */
    Byte3 accel() const {
        ASSERT(sys < NUM_SLOTS);
        _SYSByte4 v;
        v.value = _SYS_getAccel(*this);
        return vec(v.x, v.y, v.z);
    }

    /**
     * Return the 'tilt' state, derived from the raw accelerometer data
     * by a built-in filter. Tilt is a vector, where each component is
     * in the set (-1, 0, +1).
     */
    Byte2 tilt() const {
        ASSERT(sys < NUM_SLOTS);
        _SYSByte4 v;
        v.value = _SYS_getTilt(*this);
        return vec(v.x, v.y);
    }

    /**
     * Is this cube being touched right now? Return the current state
     * of the touch sensor.
     */
    bool isTouching() const {
        ASSERT(sys < NUM_SLOTS);
        return _SYS_isTouching(*this);
    }
    
    /**
     * Is a shake event being detected on this cube?
     */
    bool isShaking() const {
        ASSERT(sys < NUM_SLOTS);
        return _SYS_getShake(*this);
    }

    /**
     * Return the cube's unique 64-bit hardware ID. This ID uniquely
     * identifies the cube that this slot is paired with.
     *
     * The system caches these IDs, so usually this function will return
     * nearly instantly. However, if the HWID is not yet known, this may block
     * while we wait on a radio round-trip to discover the HWID.
     */
    uint64_t hwID() const {
        ASSERT(sys < NUM_SLOTS);
        return _SYS_getCubeHWID(*this);
    }

    /**
     * Detach any video buffer which was previously attached to this
     * cube. After this point, we'll refrain from sending any video updates
     * to this cube. The cube will retain its existing screen contents.
     *
     * Waits for all cubes to finish rendering before detaching.
     */
    void detachVideoBuffer() const {
        ASSERT(sys < NUM_SLOTS);
        _SYS_finish();
        _SYS_setVideoBuffer(*this, 0);
    }

    /**
     * Get this cube's battery level.
     *
     * XXX: Units are currently TBD.
     */
    unsigned batteryLevel() const {
        ASSERT(sys < NUM_SLOTS);
        return _SYS_getBatteryV(*this);
    }

    CubeID operator ++() { return ++sys; }
    CubeID operator ++(int) { return sys++; }
    CubeID operator --() { return --sys; }
    CubeID operator --(int) { return sys--; }
};


/**
 * A Neighborhood is a description of all neighbors for a single cube,
 * packed into a small value. Each side can be empty, or it can refer to
 * another cube by its CubeID.
 */

struct Neighborhood {
    _SYSNeighborState sys;

    /**
     * Default constructor. Leaves the Neighborhood uninitialized.
     */
    Neighborhood() {}
    
    /**
     * Initialize a Neighborhood from a low-level _SYSNeighborState object.
     */
    Neighborhood(_SYSNeighborState sys) : sys(sys) {}

    /**
     * Implicit conversion to _SYSNeighborState,
     * for use in low-level system calls.
     */
    operator _SYSNeighborState& () {
        return sys;
    }

    /**
     * Get a Neighborhood representing the physical neighbors for a cube.
     *
     * The sides in this Neighborhood are relative to the physical cube
     * hardware, not to the current screen orientation. (A CubeID object has
     * no way of knowing what the current screen orientation is.)
     */
    Neighborhood(CubeID cube) {
        ASSERT(cube < cube.NUM_SLOTS);
        sys.value = _SYS_getNeighbors(cube);
    }

    /**
     * Return the neighbor at a particular side.
     * If no neighbor exists at that side, we return an undefined CubeID.
     */
    CubeID neighborAt(Side side) const {
        ASSERT(side >= 0 && side < NUM_SIDES);
        return sys.sides[side];
    }

    /**
     * Is there a neighbor at this side? This is equivalent to calling
     * isDefined() on the result of neighborAt().
     */
    bool hasNeighborAt(Side side) const {
        return neighborAt(side).isDefined();
    }

    /**
     * If the specified cube is part of this neighborhood (i.e. it was
     * neighbored to the cube that this Neighborhood was created for),
     * return the Side where that cube is found. Otherwise, returns NO_SIDE.
     */
    Side sideOf(CubeID cube) const {
        for (Side side = (Side)0; side < NUM_SIDES; ++side)
            if (sys.sides[side] == cube)
                return side;
        return NO_SIDE;
    }
};


};  // namespace Sifteo

#endif
