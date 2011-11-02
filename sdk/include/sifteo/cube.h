/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_CUBE_H
#define _SIFTEO_CUBE_H

#include <sifteo/asset.h>
#include <sifteo/video.h>

#define CUBE_ID_UNDEFINED ((_SYSCubeID)-1)

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

    VideoBuffer vbuf;

 private:
    ID mID;
};


};  // namespace Sifteo

#endif
