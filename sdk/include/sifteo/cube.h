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
    
    /**
     * Retrieve the neighor on the given is, of NULL if it is open.
     */
    
    Cube* physicalNeighborAt(Side side) const {
        return mNeighbors[side];
    }
    
    /**
     * If the current cube is currently neighbored to this, retrieve
     * the id of it's side, otherwise return SIDE_UNDEFINED.
     */
    
    Side physicalSideOf(Cube* cube) const {
        for(Side side=0; side<NUM_SIDES; ++side) {
            if (mNeighbors[side] == cube) {
                return side;
            }
        }
        return SIDE_UNDEFINED;
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
    
    /**
     * Like physicalNeighborAt, but relative to the current LCD rotation.
     */
    
    Cube* virtualNeighborAt(Side side) const {
        Side rot = orientation();
        if (rot == SIDE_UNDEFINED) { return 0; }
        side = (side + rot) % NUM_SIDES;
        return physicalNeighborAt(side);
    }
    
    /**
     * Like physicalSideOf, but relative to the current LCD rotation.
     */
    
    Side virtualSideOf(Cube* cube) const {
        int side = physicalSideOf(cube);
        if (side == SIDE_UNDEFINED) { return SIDE_UNDEFINED; }
        Side rot = orientation();
        if (rot == SIDE_UNDEFINED) { return SIDE_UNDEFINED; }
        side = (side - rot) % NUM_SIDES;
        return side < 0 ? side + NUM_SIDES : side;
    }
    
    /**
     * Inspect the current neighbor state, and update everyone neighbors and
     * enqueue the deltas.
     * NOTE: This interface suuuuuucks, fixme!
     */
    
    static void updateNeighbors(Cube* cubes, uint8_t nCubes) {
      // coalesce pairs
      NeighborPair pairs[(nCubes-1)*(nCubes-1)];
      for(NeighborPair *p = pairs; p != pairs + (nCubes-1)*(nCubes-1); ++p) {
        p->side0 = SIDE_UNDEFINED;
        p->side1 = SIDE_UNDEFINED;
      }
      uint8_t buf[nCubes*NUM_SIDES];
      for(ID id=0; id<nCubes; ++id) {
        uint8_t* p = buf + id*NUM_SIDES;
        _SYS_getRawNeighbors(id, p);
        for(Side side=0; side<NUM_SIDES; ++side) {
          if(p[side]>>7) {
            uint8_t otherId = p[side] & 0x1f;
            if (otherId < id) {
              pairs->lookup(nCubes, otherId, id)->side1 = (int8_t)side;
            } else {
              pairs->lookup(nCubes, id, otherId)->side0 = (int8_t)side;
            }
          }
        }
      }
      // look for removes
      for(ID id=0; id<nCubes; ++id) {
        Cube* v = cubes + id;
        for(Side side=0; side<NUM_SIDES; ++side) {
          Cube* nv = v->physicalNeighborAt(side);
          if (nv) {
            NeighborPair* p;
            if (id < nv->id()) {
              p = pairs->lookup(nCubes, id, nv->id());
            } else {
              p = pairs->lookup(nCubes, nv->id(), id);
            }
            if (p->FullyDisconnected()) {
              Side otherSide = nv->physicalSideOf(v);
              v->willRemoveNeighbor(nv, side, otherSide);
              nv->willRemoveNeighbor(v, otherSide, side);
              // enqueue event: OnNeighborRemove(v, side, nv, otherSide);
            }
          }
        }
      }
      // look for adds
      for(ID id=0; id<nCubes-1; ++id) {
        for(unsigned otherId=id+1; otherId<nCubes; ++otherId) {
          NeighborPair* p = pairs->lookup(nCubes, id, otherId);
          if (p->FullyConnected()) {
            // fully connected
            Cube* v0 = cubes + id;
            Cube* v1 = cubes + otherId;
            if (!v0->physicalNeighborAt(p->side0)) {
              v0->didAddNeighbor(v1, p->side0, p->side1);
              v1->didAddNeighbor(v0, p->side1, p->side0);
              // enqueue event: OnNeighborAdd(v0, p->side0, v1, p->side1);
            }
          }
        }
      } 
    }
    
    VideoBuffer vbuf;

 private:
    ID mID;
    Cube* mNeighbors[NUM_SIDES];
    
    // used by updateNeighbors()
    struct NeighborPair {
      Side side0;
      Side side1;

      bool FullyConnected() const { return side0 != SIDE_UNDEFINED && side1 != SIDE_UNDEFINED; }
      bool FullyDisconnected() const { return side1 == SIDE_UNDEFINED && side1 == SIDE_UNDEFINED; }

      NeighborPair* lookup(int nCubes, int cid0, int cid1) {
        // invariant cid0 < cid1
        return (this + cid0 * (nCubes-1) + (cid1-1));
      }
    };
    
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
