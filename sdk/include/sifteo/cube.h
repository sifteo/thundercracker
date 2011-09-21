/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_CUBE_H
#define _SIFTEO_CUBE_H

#include <sifteo/asset.h>
#include <sifteo/video.h>

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

    Cube(ID id)
	: mID(id) {}

    void enable() {
	_SYS_setVideoBuffer(mID, &vram.sys);
	_SYS_enableCubes(Intrinsic::LZ(mID));
    }

    void disable() {
	_SYS_disableCubes(Intrinsic::LZ(mID));
    }

    void loadAssets(AssetGroup &group) {
	_SYS_loadAssets(mID, &group.sys);
    }

    ID id() const {
	return mID;
    }

    VideoBuffer vram;

 private:
    ID mID;
};


};  // namespace Sifteo

#endif
