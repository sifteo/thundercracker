/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_ABI_ASSET_H
#define _SIFTEO_ABI_ASSET_H

#include <sifteo/abi/types.h>

#ifdef __cplusplus
extern "C" {
#endif


#define _SYS_ASSETLOAD_BUF_SIZE     48      // Makes _SYSAssetLoaderCube come to 64 bytes
#define _SYS_MAX_ASSET_SLOTS        4       // Number of AssetSlots maximum per-program
#define _SYS_TILES_PER_ASSETSLOT    4096    // Number of tiles per AssetSlot
#define _SYS_ASSET_GROUPS_PER_SLOT  24      // Number of AssetGroups we can track per-slot
#define _SYS_ASSET_SLOTS_PER_BANK   4       // Number of AssetSlots maximum per-program
#define _SYS_ASSET_GROUP_SIZE_UNIT  16      // Basic unit of AssetGroup allocation, in tiles
#define _SYS_ASSET_GROUP_CRC_SIZE   16      // Number of bytes of AssetGroup CRC


struct _SYSAssetGroupHeader {
    uint8_t reserved;                       /// OUT     Reserved, must be zero
    uint8_t ordinal;                        /// OUT     Small integer, unique within an ELF
    uint16_t numTiles;                      /// OUT     Uncompressed size, in tiles
    uint32_t dataSize;                      /// OUT     Size of compressed data, in bytes
    uint8_t crc[_SYS_ASSET_GROUP_CRC_SIZE]; /// OUT     CRC of this asset group's data
    // Followed by compressed data
};

struct _SYSAssetGroupCube {
    uint16_t baseAddr;          /// IN     Installed base address, in tiles
};

struct _SYSAssetGroup {
    uint32_t pHdr;              /// OUT    Read-only data for this asset group
    // Followed by a _SYSAssetGroupCube array
};

struct _SYSAssetLoaderCube {
    uint32_t progress;      /// IN    Number of compressed bytes read from flash
    uint32_t total;         /// IN    Local copy of asset group's dataSize
    uint16_t reserved;      /// -
    uint8_t head;           /// -     Index of the next sample to read
    uint8_t tail;           /// -     Index of the next empty slot to write into
    uint8_t buf[_SYS_ASSETLOAD_BUF_SIZE];
};

struct _SYSAssetLoader {
    _SYSCubeIDVector busyCubes; /// OUT   Which cubes are still busy loading?
    // Followed by a _SYSAssetLoaderCube array
};

struct _SYSAssetConfiguration {
    uint32_t pGroup;            /// Address of _SYSAssetGroup
    _SYSVolumeHandle volume;    /// Mapped volume, or 0 if the group is local
    uint32_t dataSize;          /// Copy of group header's dataSize
    uint16_t numTiles;          /// Copy of group header's numTiles
    uint8_t ordinal;            /// Copy of group header's ordinal
    _SYSAssetSlot slot;         /// Which slot to load into?
};

enum _SYSAssetImageFormat {
    _SYS_AIF_PINNED = 0,        /// All tiles are linear. "data" is index of the first tile
    _SYS_AIF_FLAT,              /// "data" points to a flat array of 16-bit tile indices
    _SYS_AIF_DUB_I8,            /// Dictionary Uniform Block codec, 8-bit index
    _SYS_AIF_DUB_I16,           /// Dictionary Uniform Block codec, 16-bit index
};

struct _SYSAssetImage {
    uint32_t pAssetGroup;       /// Address for _SYSAssetGroup in RAM, 0 for pre-relocated
    uint16_t width;             /// Width of the asset image, in tiles
    uint16_t height;            /// Height of the asset image, in tiles
    uint16_t frames;            /// Number of "frames" in this image
    uint8_t  format;            /// _SYSAssetImageFormat
    uint8_t  reserved;          /// Reserved, must be zero
    uint32_t pData;             /// Format-specific data or data pointer
};


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
