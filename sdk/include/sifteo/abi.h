/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Definition of the Application Binary Interface for Sifteo games.
 *
 * Whereas the rest of the SDK is effectively a layer of malleable
 * syntactic sugar, this defines the rigid boundary between a game and
 * its execution environment. Everything in this file posesses a
 * binary compatibility guarantee.
 *
 * The ABI is defined in plain C, and all symbols are namespaced with
 * '_SYS' so that it's clear they aren't meant to be used directly by
 * game code. The one exception is siftmain(), the user entry point.
 */

#ifndef _SIFTEO_ABI_H
#define _SIFTEO_ABI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Data types which are valid across the user/system boundary.
 *
 * Data directions are from the userspace's perspective. IN is
 * firmware -> game, OUT is game -> firmware.
 */

#define _SYS_NUM_CUBE_SLOTS   32

typedef uint8_t _SYSCubeID;		/// Cube slot index
typedef uint32_t _SYSCubeIDVector;	/// One bit for each cube slot

/*
 * XXX: It would be nice to further compress the loadstream when storing
 *      it in the master's flash. There's a serious memory tradeoff here,
 *      though, and right now I'm assuming RAM is more important than
 *      flash to conserve. I've been using LZ77 successfully for this
 *      compressor, but that requires a large output window buffer. This
 *      is clearly an area for improvement, and we might be able to tweak
 *      an existing compression algorithm to work well. Or perhaps we put
 *      that effort into improving the loadstream codec.
 */

struct _SYSAssetGroupHeader {
    uint8_t hdrSize;		/// OUT    Size of header / offset to compressed data
    uint8_t reserved;		/// OUT    Reserved, must be zero
    uint16_t numTiles;		/// OUT    Uncompressed size, in tiles
    uint32_t dataSize;		/// OUT    Size of compressed data, in bytes
    uint64_t signature;		/// OUT    Unique identity for this group
};

struct _SYSAssetGroupCube {
    uint32_t baseAddr;		/// IN     Base address where this group is installed
    uint32_t progress;		/// IN     Loading progress, in bytes
};

struct _SYSAssetGroup {
    const struct _SYSAssetGroupHeader *hdr;	/// OUT    Static data for this asset group
    struct _SYSAssetGroupCube *cubes;		/// OUT    Array of per-cube state buffers
    _SYSCubeIDVector reqCubes;			/// IN     Which cubes have requested to load this group?
    _SYSCubeIDVector doneCubes;			/// IN     Which cubes have finished installing this group?
};

struct _SYSVideoBuffer {
    uint8_t  bytes[1024];	/// OUT    Raw cube RAM contents
    uint32_t cm64[16];		/// INOUT  Change map, at a resolution of 1 bit per 16-bit word
    uint32_t cm4;		/// INOUT  Change map, at a resolution of 1 bit per 32 bytes
};


/**
 * Event vectors. These can be changed at runtime in order to handle
 * events within the game binary. All vectors are NULL (no-op) by
 * default. The vector table lives at an agreed-upon address in
 * game-accessable RAM.
 */

#define _SYS_MAX_VECTORS	32

struct _SYSEventVectors {
    void (*cubeFound)(_SYSCubeID cid);
    void (*cubeLost)(_SYSCubeID cid);
    void (*assetDone)(struct _SYSAssetGroup *group);
    void *reserved[_SYS_MAX_VECTORS - 3];
};

extern struct _SYSEventVectors _SYS_vectors;


/**
 * Entry point to the game binary.
 */

void siftmain(void);


/**
 * Low-level system call interface.
 */
    
void _SYS_exit(void);				/// Equivalent to return from siftmain()
void _SYS_yield(void);				/// Temporarily cede control to the firmware
void _SYS_draw(void);				/// Enqueue a new rendering frame

void _SYS_enableCubes(_SYSCubeIDVector cv);	/// Which cubes will be trying to connect?
void _SYS_disableCubes(_SYSCubeIDVector cv);

void _SYS_setVideoBuffer(_SYSCubeID cid, struct _SYSVideoBuffer *vbuf);
void _SYS_loadAssets(_SYSCubeID cid, struct _SYSAssetGroup *group);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
