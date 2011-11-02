/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
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
#define SIDE_UNDEFINED (0xff)


namespace Sifteo {


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
    typedef uint8_t Side;
	
	Cube()
		: mID(CUBE_ID_UNDEFINED) {}
	
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
        for(int i=0; i<NUM_SIDES; ++i) {
            mNeighbors[i] = 0;
        }
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
        return group.cubes[id()].progress * max / group.sys.hdr->dataSize;
    }

    bool assetDone(AssetGroup &group) {
        return !!(group.sys.doneCubes & Sifteo::Intrinsic::LZ(id()));
    }   

    ID id() const {
        return mID;
    }
    
    Cube* physicalNeighborAt(Side side) const {
        return mNeighbors[side];
    }
    
    Side physicalSideOf(Cube* cube) const {
        for(Side side=0; side<NUM_SIDES; ++side) {
            if (mNeighbors[side] == cube) {
                return side;
            }
        }
        return SIDE_UNDEFINED;
    }

    VidMode::Rotation rotation() const {
        const uint8_t mask = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP;
        uint8_t flags = vbuf.peekb(offsetof(_SYSVideoRAM, flags));
        return (VidMode::Rotation)(flags & mask);
    }
    
    Side orientation() const {
        switch(rotation()) {
            case VidMode::ROT_NORMAL: return SIDE_TOP;
            case VidMode::ROT_LEFT_90: return SIDE_LEFT;
            case VidMode::ROT_180: return SIDE_BOTTOM;
            case VidMode::ROT_RIGHT_90: return SIDE_RIGHT;
            default: return SIDE_UNDEFINED;
        }
    }
    
    Cube* virtualNeighborAt(Side side) const {
        Side rot = orientation();
        if (rot == SIDE_UNDEFINED) { return 0; }
        side = (side + rot) % NUM_SIDES;
        return physicalNeighborAt(side);
    }
    
    Side virtualSideOf(Cube* cube) const {
        int side = physicalSideOf(cube);
        if (side == SIDE_UNDEFINED) { return SIDE_UNDEFINED; }
        Side rot = orientation();
        if (rot == SIDE_UNDEFINED) { return SIDE_UNDEFINED; }
        side = (side - rot) % NUM_SIDES;
        return side < 0 ? side + NUM_SIDES : side;
    }
    
    VideoBuffer vbuf;

 private:
    ID mID;
    Cube* mNeighbors[NUM_SIDES];
    
    void didAddNeighbor(Cube* cube, Side mySide, Side theirSide) {
        ASSERT(mNeighbors[mySide] == 0);
        mNeighbors[mySide] = cube;
    }
    
    void willRemoveNeighbor(Cube* cube, Side mySide, Side theirSide) {
        ASSERT(mNeighbors[mySide] == cube);
        mNeighbors[mySide] = 0;
    }
    
};


};  // namespace Sifteo

#endif
