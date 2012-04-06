/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "imagedecoder.h"
#include "cube.h"
#include "cubeslots.h"


bool ImageDecoder::init(const _SYSAssetImage *userPtr, _SYSCubeID cid)
{
    /*
     * Validate user pointers, and load the header and base address for
     * this AssetImage on this cube.
     *
     * Returns 'true' on success, 'false' on failure due to bad
     * userspace-provided data.
     */

    if (!SvmMemory::copyROData(header, userPtr))
        return false;

    if (cid >= _SYS_NUM_CUBE_SLOTS)
        return false;
    CubeSlot &cube = CubeSlots::instances[cid];

    // Map and validate per-cube data.
    // We don't need to map the _SYSAssetGroup itself- this is still a raw user pointer
    _SYSAssetGroup *userGroupPtr = reinterpret_cast<_SYSAssetGroup*>(header.pAssetGroup);
    _SYSAssetGroupCube *gc = cube.assetGroupCube(userGroupPtr);
    if (!gc)
        return false;
    baseAddr = gc->baseAddr;

    // Other member initialization
    blockCache.index = -1;

    return true;
}

int ImageDecoder::tile(unsigned x, unsigned y, unsigned frame)
{
    if (x >= header.width || y >= header.height || frame >= header.frames)
        return NO_TILE;

    switch (header.format) {

        // Sequential tiles
        case _SYS_AIF_PINNED: {
            unsigned location = x + (y + frame * header.height) * header.width;
            return header.data + baseAddr + location;
        }

        // Uncompressed tile array
        case _SYS_AIF_FLAT: {
            unsigned location = x + (y + frame * header.height) * header.width;
            uint16_t tile;
            if (SvmMemory::copyROData(tile, header.data + (location << 1)))
                return tile + baseAddr;
            return NO_TILE;
        }

        // Compressed tile array, with 8x8x1 block size
        case _SYS_AIF_DUB_I8:
        case _SYS_AIF_DUB_I16: {
            // Size of image, in blocks
            unsigned xBlocks = (header.width + 7) >> 3;
            unsigned yBlocks = (header.height + 7) >> 3;

            // Which block is this tile in?
            unsigned bx = x >> 3, by = y >> 3;
            unsigned blockNum = bx + (by + frame * yBlocks) * xBlocks;

            // How wide is the selected block?
            unsigned blockW = MIN(8, header.width - (bx & ~7));

            if (blockCache.index != blockNum)
                decompressDUB(blockNum);

            return blockCache.data[(bx & 7) + (by & 7) * blockW];
        }

        default: {
            return NO_TILE;
        }
    }
}

void ImageDecoder::decompressDUB(unsigned index)
{
    blockCache.index = index;
}
