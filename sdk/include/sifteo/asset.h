/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_ASSET_H
#define _SIFTEO_ASSET_H

#include <stdint.h>

namespace Sifteo {


/**
 * An asset group. At build time, STIR creates a static instance of
 * the AssetGroup class for every asset group in the game.
 *
 * At runtime, asset groups contain static data used to program the
 * group's assets into the cubes.
 *
 * XXX: We need a good place to store the base address of this group.
 *      That will need to be kept per-cube.
 *
 * XXX: It would be nice to further compress the loadstream when storing
 *      it in the master's flash. There's a serious memory tradeoff here,
 *      though, and right now I'm assuming RAM is more important than
 *      flash to conserve. I've been using LZ77 successfully for this
 *      compressor, but that requires a large output window buffer. This
 *      is clearly an area for improvement, and we might be able to tweak
 *      an existing compression algorithm to work well. Or perhaps we put
 *      that effort into improving the loadstream codec.
 */

struct AssetGroup {
    /* Data initialized by STIR */
    const uint8_t *mStream;
    uint32_t mStreamLen;
};


};  // namespace Sifteo

#endif
