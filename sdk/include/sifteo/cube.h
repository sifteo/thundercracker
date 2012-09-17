/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/limits.h>
#include <sifteo/macros.h>
#include <sifteo/math.h>
#include <sifteo/array.h>

namespace Sifteo {

/**
 * @defgroup cube Cube
 *
 * @brief Objects for cube identity and cube sensor state
 *
 * @{
 */

/**
 * @brief An enumeration which names the four sides of a Sifteo cube.
 *
 * Depending on context, these names can refer to either "physical" sides
 * (relative to the hardware in a cube) or "virtual" sides (relative to the
 * current screen rotation.)
 */

enum Side {
    TOP = 0,            ///< Top side (Y-)
    LEFT,               ///< Left side (X-)
    BOTTOM,             ///< Bottom side (Y+)
    RIGHT,              ///< Right side (X+)
    NUM_SIDES,          ///< Total number of sides (4)
    NO_SIDE = -1        ///< Nil value (-1)
};

/**
 * @brief Alternate POD type for CubeID storage
 *
 * CubeID is the main Cube data type that games should be using, but it
 * is not a POD type. PCubeID is a POD type that can be used to store
 * cube IDs, and to construct a CubeID when necessary.
 */
typedef _SYSCubeID PCubeID;

/**
 * @brief A lightweight identifier for one Sifteo cube.
 *
 * This is a small unsigned integer which refers to a "cube slot", a
 * collection of resources that the operating system maintains for each cube
 * or potential cube.
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
     * @brief Default constructor. By default, a CubeID is initialized to a
     * special Undefined value. This value can be tested for with the
     * isDefined() predicate.
     */
    CubeID() : sys(UNDEFINED) {}

    /**
     * @brief Initialize a CubeID with a concrete slot value. Slots are numbered
     * from 0 to NUM_SLOTS - 1.
     */
    CubeID(_SYSCubeID sys) : sys(sys) {}

    /**
     * @brief Implicit conversion to _SYSCubeID, for use in low-level system calls.
     */
    operator _SYSCubeID() const {
        return sys;
    }

    /**
     * @brief Return the _SYSCubeIDVector bit associated with this ID.
     *
     * These bits are used for some system calls, and they can be used to quickly
     * describe a set of cubes.
     */
     
    _SYSCubeIDVector bit() const {
        return 0x80000000 >> sys;
    }
    
    /**
     * @brief Is this a CubeID that was initialized with a valid slot number, and
     * not one that was initialized as undefined?
     */
    bool isDefined() const {
        return sys != UNDEFINED;
    }

    /**
     * @brief Return the physical accelerometer state, as a signed byte-vector.
     *
     * All components are signed bytes, ranging from -128 to 127, with a full
     * scale range of 2G. +X is to the right, +Y is toward the bottom of the screen,
     * and +Z points "into" the screen.
     *
     * A vector of (0, 0, 0) is considered free-fall. A cube lying flat on the table
     * will have X and Y coordinates near zero, and a positive Z coordinate near 64.
     * An upside-down cube would have a negative Z coordinate near -64.
     */
    Byte3 accel() const {
        ASSERT(sys < NUM_SLOTS);
        _SYSByte4 v;
        v.value = _SYS_getAccel(*this);
        return vec(v.x, v.y, v.z);
    }

    /**
     * @brief Is this cube being touched right now? Return the current state
     * of the touch sensor.
     */
    bool isTouching() const {
        ASSERT(sys < NUM_SLOTS);
        return _SYS_isTouching(*this);
    }
    
    /**
     * @brief Return the cube's unique 64-bit hardware ID. This ID uniquely
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
     * @brief Detach any video buffer which was previously attached to this cube.
     *
     * After this point, we'll refrain from sending any video updates
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
     * @brief Detach any motion buffer which was previously attached to this cube.
     *
     * After this point, we'll refrain from capturing high-frequency motion
     * data from this cube. The latest motion sample will still be available
     * from accel().
     */
    void detachMotionBuffer() const {
        ASSERT(sys < NUM_SLOTS);
        _SYS_setMotionBuffer(*this, 0);
    }

    /**
     * @brief Get this cube's battery level.
     *
     * The battery level has already been converted into a "fuel gauge"
     * representation, with 0.0f representing a qualitatively dead
     * battery and 1.0f representing a totally new battery.
     *
     * A reading of exactly zero will only result when the battery level is not
     * yet known. It takes the cube about a second to measure its own battery
     * level, so if you're talking to a cube that has just been turned on and
     * connected, the first measurement may not yet be available.
     *
     * To get the battery level for the base unit, call System::batteryLevel().
     */
    float batteryLevel() const {
        ASSERT(sys < NUM_SLOTS);
        return _SYS_cubeBatteryLevel(*this) / float(_SYS_BATTERY_MAX);
    }

    /**
     * @brief Remove the persistent pairing association between this cube and the current Base.
     *
     * This cube will also be shutdown and disconnected.
     */
    void unpair() const {
        ASSERT(sys < NUM_SLOTS);
        _SYS_unpair(*this);
    }

    CubeID operator ++() { return ++sys; }
    CubeID operator ++(int) { return sys++; }
    CubeID operator --() { return --sys; }
    CubeID operator --(int) { return sys--; }
};


/**
 * @brief An unordered set of cubes
 *
 * A CubeSet can be used with API functions anywhere a _SYSCubeIDVector is
 * expected, and you can iterate over a CubeSet using C++11 style iteration.
 */

class CubeSet : public BitArray<_SYS_NUM_CUBE_SLOTS> {
public:
    /// Implicit conversion to _SYSCubeIDVector, for use in low-level system calls.
    operator _SYSCubeIDVector() const {
        return words[0];
    }

    /// Implicit conversion from a BitArray of the correct size
    CubeSet(const BitArray<_SYS_NUM_CUBE_SLOTS> &bits) : BitArray<_SYS_NUM_CUBE_SLOTS>(bits) {}

    /// Create an empty CubeSet
    explicit CubeSet() : BitArray<_SYS_NUM_CUBE_SLOTS>() {}

    /// Create a CubeSet with a single CubeID in it.
    explicit CubeSet(CubeID cube) : BitArray<_SYS_NUM_CUBE_SLOTS>(cube) {}

    /**
     * @brief Create a new CubeSet with a range of cubes.
     *
     * This is a half-open interval. All IDs >= 'begin' and < 'end' are in the set.
     */
    explicit CubeSet(CubeID begin, CubeID end) : BitArray<_SYS_NUM_CUBE_SLOTS>(begin, end) {}

    /**
     * @brief Getter for the underlying bitmask
     * 
     * Use CubeID::bit() to match a cube to this.
     */
     uint32_t mask() const { 
         return words[0]; 
     }
     
     /**
      * @brief Setter for the underlying bitmask
      *
      * Validates that the bit is in the CUBE_ALLOCATION range,
      * but not necessarily connected.
      */
      void setMask(uint32_t mask) {
          ASSERT( (mask & ((1<<(32-CUBE_ALLOCATION))-1)) == 0 );
          words[0] = mask;
      }

    /**
     * @brief Return a CubeSet containing all connected cubes which are visible to the
     * current application.
     *
     * The number of available cubes will always be within the limits set by your
     * Metadata::cubeRange(). Furthermore, returned cube IDs are all guaranteed to
     * be less than the maximum number of cubes defined in your range. If your
     * code supports at most 6 cubes, for instance, it's perfectly fine to declare
     * 6-element arrays and index them with any CubeID in this set.
     *
     * The returned CubeSet is guaranteed not to change until the relevant
     * connection and/or disconnection events have been dispatched to your application.
     */
    static CubeSet connected() {
        CubeSet result;
        result.words[0] = _SYS_getConnectedCubes();
        return result;
    }
};


/**
 * @brief A lightweight identifier for one neighbored object.
 *
 * Neighbor IDs represent anything that can be detected by one of a cube's
 * four neighbor sensors. Currently this could be another cube, a base,
 * or nothing at all.
 */

struct NeighborID {
    _SYSNeighborID sys;

    /**
     * @brief Default constructor. Initializes an empty NeighborID.
     */
    NeighborID() : sys(_SYS_NEIGHBOR_NONE) {}

    /**
     * @brief Initialize a NeighborID with a concrete value.
     */
    NeighborID(_SYSNeighborID sys) : sys(sys) {}

    /**
     * @brief Implicit conversion to _SYSNeighborID, for use in low-level system calls.
     */
    operator _SYSNeighborID() const {
        return sys;
    }

    /// Is this neighbor a cube?
    bool isCube() const {
        return sys < _SYS_NUM_CUBE_SLOTS;
    }

    /// Is this neighbor a base?
    bool isBase() const {
        return (sys & _SYS_NEIGHBOR_TYPE_MASK) == _SYS_NEIGHBOR_BASE;
    }

    /// Is there nothing neighbored at all?
    bool isEmpty() const {
        return sys == _SYS_NEIGHBOR_NONE;
    }

    /**
     * @brief Convert this NeighborID to a CubeID
     *
     * If the neighbor is not a cube, returns an 'undefined' CubeID.
     */
    CubeID cube() const {
        return isCube() ? CubeID(sys) : CubeID();
    }
};


/**
 * @brief A Neighborhood is a description of all neighbors for a single cube,
 * packed into a small value.
 *
 * Each side can be empty, or it can refer to
 * another cube by its CubeID.
 */

struct Neighborhood {
    _SYSNeighborState sys;

    /**
     * @brief Default constructor. Leaves the Neighborhood uninitialized.
     */
    Neighborhood() {}
    
    /**
     * @brief Initialize a Neighborhood from a low-level _SYSNeighborState object.
     */
    Neighborhood(_SYSNeighborState sys) : sys(sys) {}

    /**
     * @brief Implicit conversion to _SYSNeighborState,
     * for use in low-level system calls.
     */
    operator _SYSNeighborState& () {
        return sys;
    }

    /**
     * @brief Get a Neighborhood representing the physical neighbors for a cube.
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
     * @brief Return the neighbor at a particular side, as a NeighborID
     */
    NeighborID neighborAt(Side side) const {
        ASSERT(side >= 0 && side < NUM_SIDES);
        return sys.sides[side];
    }

    /**
     * @brief Return the neighbor at a particular side, as a CubeID.
     *
     * If no neighbor exists at that side OR if the neighbor is not a cube,
     * we return an undefined CubeID.
     */
    CubeID cubeAt(Side side) const {
        return neighborAt(side).cube();
    }

    /**
     * @brief Is there a neighbor at this side? This is equivalent to calling
     * !isEmpty() on the result of neighborAt().
     */
    bool hasNeighborAt(Side side) const {
        return !neighborAt(side).isEmpty();
    }

    /**
     * @brief Is there a cube at this side? This is equivalent to calling
     * isCube() on the result of neighborAt().
     */
    bool hasCubeAt(Side side) const {
        return neighborAt(side).isCube();
    }

    /**
     * @brief Search for a CubeID in this Neighborhood
     *
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

/**
 * @} endgroup cube
 */

};  // namespace Sifteo
