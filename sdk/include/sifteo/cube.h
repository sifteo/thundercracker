/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_CUBE_H
#define _SIFTEO_CUBE_H

#include <sifteo/asset.h>
#include <sifteo/video.h>

#define CUBE_ID_UNDEFINED (0xff)
#define NUM_SIDES (4)
#define SIDE_TOP (0)
#define SIDE_LEFT (1)
#define SIDE_BOTTOM (2)
#define SIDE_RIGHT (3)
#define SIDE_UNDEFINED (-1)


namespace Sifteo {

/**
 * These lookup tables should be relocated to an external translation unit;
 * their inclusion here is tomporary.
 * 
 * Also todo: replace raw neighbors with coalesced neighbors onces that's up and
 * working.
 */

// unit vectors for directions
static const Vec2 kSideToUnit[4] = {
  Vec2(0, -1),
  Vec2(-1, 0),
  Vec2(0, 1),
  Vec2(1, 0)
};

// complex rotation vectors for directions
static const Vec2 kSideToQ[4] = { 
  Vec2(1,0),
  Vec2(0,1),
  Vec2(-1,0),
  Vec2(0,-1)
};

#ifdef SIFTEO_SIMULATOR

// internal -- used by setOrientation()
static const VidMode::Rotation kSideToRotation[4] = {
  VidMode::ROT_NORMAL,
  VidMode::ROT_LEFT_90,
  VidMode::ROT_180,
  VidMode::ROT_RIGHT_90
};

#else
// internal -- used by setOrientation()
static const VidMode::Rotation kSideToRotation[4] = {
  VidMode::ROT_NORMAL,
  VidMode::ROT_RIGHT_90,
  VidMode::ROT_180,
  VidMode::ROT_LEFT_90
};

#endif

// internal -- used by orientTo()
static const int kOrientationTable[4][4] = {
  {2,1,0,3},
  {3,2,1,0},
  {0,3,2,1},
  {1,0,3,2}
};


/**
 * Represents one Sifteo cube. These Cube objects are statically
 * allocated, and really they represent a slot for a potential cube in
 * your game. If you only work with three cubes, you only need to
 * allocate three cube objects.  Every cube starts out in a
 * disconnected state.
 */

class Cube {
 public:
    typedef _SYSCubeID ID;
    typedef _SYSSideID Side;
    typedef _SYSNeighborState Neighborhood;
	typedef _SYSTiltState  TiltState;

	/*
	 * Default constructor leaves mID zero'ed, so that Cube objects
	 * are allocated in the BSS segment rather than read-write data.
	 */

	Cube() {}
    Cube(ID id)
        : mID(id) {}

    /**
     * Prepare this cube for use. Tell the system to start trying to
     * connect to a cube, and initialize the VideoBUffer.
     */

    void enable(ID id=CUBE_ID_UNDEFINED) {
		if (id != CUBE_ID_UNDEFINED) {
			mID = id;
		}
        ASSERT(mID != CUBE_ID_UNDEFINED);
        vbuf.init();
        _SYS_setVideoBuffer(mID, &vbuf.sys);
        _SYS_enableCubes(Intrinsic::LZ(mID));
    }

    void disable() {
        _SYS_disableCubes(Intrinsic::LZ(mID));
    }
    
    void loadAssets(AssetGroup &group) {
        _SYS_loadAssets(mID, &group.sys);
    }

    /**
     * Get the asset loading progress, on this cube, scaled between 0 and 'max'.
     * By default, this returns percent complete.
     */
    
    int assetProgress(AssetGroup &group, int max=100) {
        ASSERT(id() < arraysize(group.cubes));
        return group.cubes[id()].progress * max / group.sys.size;
    }
    
    bool assetDone(AssetGroup &group) {
        return !!(group.sys.doneCubes & Sifteo::Intrinsic::LZ(id()));
    }

    ID id() const {
        return mID;
    }
    
    /**
     * Retrieve the neighor on the given is, of CUBE_ID_UNDEFINED if it is open.
     */
    
    ID physicalNeighborAt(Side side) const {
        ASSERT(side >= 0);
        ASSERT(side < NUM_SIDES);
        _SYSNeighborState state;
		_SYS_getNeighbors(mID, &state);
        return state.sides[side];
    }
    
    ID hasPhysicalNeighborAt(Side side) const {
        return physicalNeighborAt(side) != CUBE_ID_UNDEFINED;
    }
    
    /**
     * If the current cube is currently neighbored to this, retrieve
     * the id of it's side, otherwise return SIDE_UNDEFINED.
     */
    
    Side physicalSideOf(ID cube) const {
        ASSERT(cube < _SYS_NUM_CUBE_SLOTS);
        _SYSNeighborState state;
		_SYS_getNeighbors(mID, &state);
        for(Side side=0; side<NUM_SIDES; ++side) {
            if (state.sides[side] == cube) { return side; }
        }
        return SIDE_UNDEFINED;
    }
    
	Vec2 physicalAccel() const {
	  _SYSAccelState state;
	  _SYS_getAccel(mID, &state);
	  return Vec2(state.x, state.y);
	}

	TiltState getTiltState() const {
		TiltState state;
		_SYS_getTilt(mID, &state);

		return state;
	}

    /**
     * Retrieve the current LCD rotation from the video buffer.
     */
    
    VidMode::Rotation rotation() const {
        const uint8_t mask = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP;
        uint8_t flags = vbuf.peekb(offsetof(_SYSVideoRAM, flags));
        return (VidMode::Rotation)(flags & mask);
    }
    
    /**
     * Map the LCD rotation mask to screen orientation.  This is the side
     * which maps to the physical "top" of the screen.
     */
    
    Side orientation() const {
      #ifdef SIFTEO_SIMULATOR
        switch(rotation()) {
            case VidMode::ROT_NORMAL: return SIDE_TOP;
            case VidMode::ROT_LEFT_90: return SIDE_LEFT;
            case VidMode::ROT_180: return SIDE_BOTTOM;
            case VidMode::ROT_RIGHT_90: return SIDE_RIGHT;
            default: return SIDE_UNDEFINED;
        }
      #else
        switch(rotation()) {
            case VidMode::ROT_NORMAL: return SIDE_TOP;
            case VidMode::ROT_RIGHT_90: return SIDE_LEFT;
            case VidMode::ROT_180: return SIDE_BOTTOM;
            case VidMode::ROT_LEFT_90: return SIDE_RIGHT;
            default: return SIDE_UNDEFINED;
        }
      #endif
    }
    
	void setOrientation(Side topSide) {
		ASSERT(topSide >= 0);
        ASSERT(topSide < 4);
	  	VidMode mode(vbuf);
	  	mode.setRotation(kSideToRotation[topSide]);
	}
	
	void orientTo(const Cube& src) {
		Side srcSide = src.physicalSideOf(mID);
		Side dstSide = physicalSideOf(src.mID);
		ASSERT(srcSide != SIDE_UNDEFINED);
		ASSERT(dstSide != SIDE_UNDEFINED);
		srcSide = (srcSide - src.orientation()) % NUM_SIDES;
		if (srcSide < 0) { srcSide += NUM_SIDES; }
		setOrientation(kOrientationTable[dstSide][srcSide]);
	}
	
	Side physicalToVirtual(Side side) const {
        if (side == SIDE_UNDEFINED) { return SIDE_UNDEFINED; }
        ASSERT(side >= 0);
        ASSERT(side < 4);
        Side rot = orientation();
        ASSERT(rot != SIDE_UNDEFINED);
        side = (side - rot) % NUM_SIDES;
        return side < 0 ? side + NUM_SIDES : side;
	}
	
	Side virtualToPhysical(Side side) const {
        if (side == SIDE_UNDEFINED) { return SIDE_UNDEFINED; }
        ASSERT(side >= 0);
        ASSERT(side < 4);
        Side rot = orientation();
        ASSERT(rot != SIDE_UNDEFINED);
        return (side + rot) % NUM_SIDES;
	}
	
    /**
     * Like physicalNeighborAt, but relative to the current LCD rotation.
     */
    
    ID virtualNeighborAt(Side side) const {
        return physicalNeighborAt(virtualToPhysical(side));
    }
    
    ID hasVirtualNeighborAt(Side side) const {
        return virtualNeighborAt(side) != CUBE_ID_UNDEFINED;
    }
    
    /**
     * Like physicalSideOf, but relative to the current LCD rotation.
     */
    
    Side virtualSideOf(ID cube) const {
        return physicalToVirtual(physicalSideOf(cube));
    }
    
	Vec2 virtualAccel() const {
		Side rot = orientation();
		ASSERT(rot != SIDE_UNDEFINED);
	  	return physicalAccel() * kSideToQ[rot];
	}

    bool touching() const {
        return _SYS_isTouching(mID);
    }

    VideoBuffer vbuf;

 private:
    ID mID;
};


};  // namespace Sifteo

#endif
