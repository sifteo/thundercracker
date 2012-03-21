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

// unit vectors for directions
static const Byte2 kSideToUnit[4] = {
  {  0, -1 },
  { -1,  0 },
  {  0,  1 },
  {  1,  0 }
};

// complex rotation vectors for directions
static const Byte2 kSideToQ[4] = { 
  {  1,  0 },
  {  0,  1 },
  { -1,  0 },
  {  0, -1 }
};

// internal -- used by setOrientation()
static const uint8_t kSideToRotation[4] = {
  VidMode::ROT_NORMAL,
  VidMode::ROT_RIGHT_90,
  VidMode::ROT_180,
  VidMode::ROT_LEFT_90
};

// internal -- used by orientTo()
static const uint8_t kOrientationTable[4][4] = {
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

    /**
     * Default constructor leaves id() zero'ed, so that Cube objects
     * are allocated in the BSS segment rather than read-write data.
     */

    Cube() {}
    Cube(ID id) {
        vbuf.cubeID = id;
    }
    
    /**
     * Return the _SYSCubeIDVector bit for this cube, useful for making
     * low-level system calls which require this type of ID.
     */
     
    _SYSCubeIDVector bit() const {
        return 0x80000000 >> id();
    }

    /**
     * Prepare this cube for use. Tell the system to start trying to
     * connect to a cube, and initialize the VideoBUffer.
     */

    void enable(ID newID=CUBE_ID_UNDEFINED) {
        if (newID != CUBE_ID_UNDEFINED) {
            vbuf.cubeID = newID;
        }
        ASSERT(id() != CUBE_ID_UNDEFINED);
        vbuf.init();
        _SYS_setVideoBuffer(id(), &vbuf.sys);
        _SYS_enableCubes(bit());
    }

    void disable() {
        _SYS_disableCubes(bit());
    }
    
    void loadAssets(AssetGroup &group) {
        _SYS_loadAssets(id(), &group.sys);
    }

    /**
     * Get the asset loading progress, on this cube, scaled between 0 and 'max'.
     * By default, this returns percent complete.
     */
    
    int assetProgress(AssetGroup &group, int max=100) {
        _SYSAssetGroupHeader *hdr = reinterpret_cast<_SYSAssetGroupHeader*>(group.sys.pHdr);
        ASSERT(id() < arraysize(group.cubes));
        return group.cubes[id()].progress * max / hdr->dataSize;
    }
    
    bool assetDone(AssetGroup &group) {
        return !!(group.sys.doneCubes & bit());
    }

    ID id() const {
        // Our vbuf already stores a cube ID.
        return vbuf.cubeID;
    }
    
    /**
     * Retrieve the neighor on the given is, of CUBE_ID_UNDEFINED if it is open.
     */
    
    ID physicalNeighborAt(Side side) const {
        ASSERT(side >= 0);
        ASSERT(side < NUM_SIDES);
        _SYSNeighborState state;
        _SYS_getNeighbors(id(), &state);
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
            _SYS_getNeighbors(id(), &state);
        for(Side side=0; side<NUM_SIDES; ++side) {
            if (state.sides[side] == cube) { return side; }
        }
        return SIDE_UNDEFINED;
    }
    
    Byte2 physicalAccel() const {
        _SYSAccelState state = _SYS_getAccel(id());
        return Vec2(state.x, state.y);
    }

    TiltState getTiltState() const {
        return _SYS_getTilt(id());
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
        switch(rotation()) {
            case VidMode::ROT_NORMAL: return SIDE_TOP;
            case VidMode::ROT_LEFT_90: return SIDE_LEFT;
            case VidMode::ROT_180: return SIDE_BOTTOM;
            case VidMode::ROT_RIGHT_90: return SIDE_RIGHT;
            default: return SIDE_UNDEFINED;
        }
    }
    
    void setOrientation(Side topSide) {
        ASSERT(topSide >= 0);
        ASSERT(topSide < 4);
        VidMode mode(vbuf);
        mode.setRotation(VidMode::Rotation(kSideToRotation[topSide]));
    }
    
    void orientTo(const Cube& src) {
        Side srcSide = src.physicalSideOf(id());
        Side dstSide = physicalSideOf(src.id());
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
    
    Byte2 virtualAccel() const {
        Side rot = orientation();
        ASSERT(rot != SIDE_UNDEFINED);
        return physicalAccel() * kSideToQ[rot];
    }

    bool touching() const {
        return _SYS_isTouching(id());
    }
    
    /**
     * Return the cube's unique 48-bit hardware ID. This ID uniquely
     * identifies the cube that this slot is paired with.
     *
     * The system caches these IDs, so usually this function will return
     * nearly instantly. However, if the HWID is not yet known, this may block
     * while we wait on a radio round-trip to discover the HWID.
     */

    uint64_t hardwareID() const {
        return _SYS_getCubeHWID(id());
    }

    VideoBuffer vbuf;
};


};  // namespace Sifteo

#endif
