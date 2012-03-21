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
    CubeID(_SYSCubeID sys) : sys(sys) {
        ASSERT(sys < NUM_SLOTS);
    }

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
     * Put the cube slot represented by this CubeID into the "enabled" state.
     * When a slot is enabled, the system will be trying to communicate with
     * this cube over the radio.
     */

    void enable() const {
        _SYS_enableCubes(bit());
    }

    /**
     * Take the cube slot represented by this CubeID out of the "enabled" state.
     * The system will no longer be in radio contact with this cube.
     */

    void disable() {
        _SYS_disableCubes(bit());
    }

    /**
     * Return the physical accelerometer state, as a signed byte-vector.
     */

    Byte2 getAccel() const {
        _SYSAccelState state = _SYS_getAccel(sys);
        return Vec2(state.x, state.y);
    }

    /**
     * Return the 'tilt' state, derived from the raw accelerometer data
     * by a built-in filter. Tilt is a vector, where each component is
     * in the set (-1, 0, +1).
     */

    Byte2 getTilt() const {
        _SYSTiltState tilt = _SYS_getTilt(sys);
        return Vec2(tilt.x - _SYS_TILT_NEUTRAL, tilt.y - _SYS_TILT_NEUTRAL);
    }

    /**
     * Is this cube being touched right now? Return the current state
     * of the touch sensor.
     */

    bool isTouching() const {
        return _SYS_isTouching(sys);
    }

    /**
     * Return the cube's unique 64-bit hardware ID. This ID uniquely
     * identifies the cube that this slot is paired with.
     *
     * The system caches these IDs, so usually this function will return
     * nearly instantly. However, if the HWID is not yet known, this may block
     * while we wait on a radio round-trip to discover the HWID.
     */

    uint64_t getHWID() const {
        return _SYS_getCubeHWID(sys);
    }
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
     * Get a Neighborhood representing the physical neighbors for a cube.
     *
     * The sides in this Neighborhood are relative to the physical cube
     * hardware, not to the current screen orientation. (A CubeID object has
     * no way of knowing what the current screen orientation is.)
     */

    Neighborhood(CubeID cube) {
        _SYS_getNeighbors(cube, &sys);
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
