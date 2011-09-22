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
typedef uint32_t _SYSCubeIDVector;	/// One bit for each cube slot, MSB-first

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

/*
 * _SYSVideoRAM is a representation of the 1KB of RAM resident in each
 * cube, which is used for storing the state of its graphics engine.
 *
 * The combination of this VRAM and the cube's current flash memory
 * contents must contain all information necessary to compose a frame
 * of graphics.
 *
 * The specific layout of this RAM may change depending on video mode
 * settings. This union describes multiple different ways to access
 * the VRAM.
 */

#define _SYS_VRAM_SIZE		1024	// Total size of VRAM, in bytes
#define _SYS_VRAM_BG0_WIDTH	18	// Width/height of BG0 tile grid
#define _SYS_VRAM_BG1_WIDTH	16	// Width/height of BG1 bitmap
#define _SYS_VRAM_BG1_TILES	144	// Total number of opaque tiles in BG1
#define _SYS_VRAM_SPRITES	8	// Maximum number of linear sprites
#define _SYS_CHROMA_KEY		0xF5	// Chroma key byte

// Bits for 'flags'

#define _SYS_VF_TOGGLE		0x02	// Toggle bit, to trigger a new frame render
#define _SYS_VF_SYNC		0x04	// Sync with LCD vertical refresh
#define _SYS_VF_CONTINUOUS	0x08	// Render continuously, without waiting for toggle
#define _SYS_VF_RESERVED_2	0x10
#define _SYS_VF_XY_SWAP		0x20	// Swap X and Y axes during render
#define _SYS_VF_X_FLIP		0x40	// Flip X axis during render
#define _SYS_VF_Y_FLIP		0x80	// Flip Y axis during render

// Values for 'mode'

#define _SYS_VM_MASK		0x1c	// Mask of valid bits in VM_MASK

#define _SYS_VM_POWERDOWN	0x00	// Power saving mode, LCD is off
#define _SYS_VM_BG0_ROM		0x04	// BG0, with tile data from internal ROM
#define _SYS_VM_FB32		0x08	// 32x32 pixel 16-color framebuffer
#define _SYS_VM_FB64		0x0c	// 64x64 pixel 2-color framebuffer
#define _SYS_VM_SOLID		0x10	// Solid color, from 'color'
#define _SYS_VM_BG0		0x14	// Background BG0: 18x18 grid
#define _SYS_VM_BG0_BG1		0x18    // BG0, plus overlay BG1: 16x16 bitmap + 144 indices
#define _SYS_VM_BG0_SPR_BG1	0x1c	// BG0, multiple linear sprites, then BG1

// Important VRAM addresses

#define _SYS_VA_COLORMAP	0x200
#define _SYS_VA_COLOR		0x3f8
#define _SYS_VA_MODE		0x3fe
#define _SYS_VA_FLAGS		0x3ff

struct _SYSSpriteInfo {
    uint16_t tile;			// 0x00
    uint8_t mask_x;			// 0x02
    uint8_t mask_y;			// 0x03
    uint8_t pos_x;			// 0x04
    uint8_t pos_y;			// 0x05
};

union _SYSVideoRAM {
    uint8_t bytes[1024];
    uint16_t words[512];

    struct {
	uint16_t bg0_tiles[324];	// 0x000 - 0x287
	uint16_t bg1_tiles[144];	// 0x288 - 0x3a7
	uint16_t bg1_bitmap[16];	// 0x3a8 - 0x3c7
	struct _SYSSpriteInfo spr[8];	// 0x3c8 - 0x3f7
	uint16_t color;			// 0x3f8 - 0x3f9
	uint8_t bg1_x;			// 0x3fa
	uint8_t bg1_y;			// 0x3fb
	uint8_t bg0_x;			// 0x3fc
	uint8_t bg0_y;			// 0x3fd
	uint8_t mode;			// 0x3fe
	uint8_t flags;			// 0x3ff
    };

    struct {
	uint8_t fb[512];		// 0x000 - 0x1ff
	uint16_t colormap[16];		// 0x200 - 0x21f
    };
};

/*
 * The _SYSVideoBuffer is a low-level data structure that serves to
 * collect graphics state updates so that the system can send them
 * over the radio to cubes.
 *
 * These buffers allow the system to send graphics data at the most
 * optimal time, in the most optimal format, and without any
 * unnecessary redundancy.
 *
 * VRAM is organized as a 1 kB array of bytes or words. We keep a few
 * bitmaps at different resolutions, for the synchronization protocol
 * below.  These bitmaps are ordered MSB-first.
 *
 * To support the system's asynchronous access requirements, these
 * buffers have a very strict synchronization protocol that user code
 * must observe:
 *
 *  1. Set the 'lock' bit(s) for any words you plan to update,
 *     using Atomic::Store().
 *
 *  2. Update the VRAM buffer, using any method and in any order you like.
 *
 *  3. Set change bits in cm1, using Atomic::Or()
 *
 *  4. Set change bits in cm32, using Atomic::Or(). After this point the
 *     system may start sending data over the radio at any time.
 *
 *  5. As soon after step (4), clear the lock bits you set in (1). Since
 *     the system treats lock bits as read-only, you can do this using
 *     Atomic::Store() again.
 *
 * Note that cm32 is the primary trigger that the system uses in order
 * to determine if hardware VRAM needs to be updated. The lock bits do
 * not prevent the system from reading VRAM, but rather they're used as
 * part of the following invariant:
 *
 *  - For any given word, we are guaranteed that the hardware's value
 *    matches the current buffered value if and only if the 'lock' bit
 *    is zero and the 'cm1' bit is zero.
 *
 * If not for the lock bits, this invariant would not be possible to
 * maintain, since the system would have no idea when userspace code
 * has modified the buffer, if it hasn't yet set the cm1 bits.
 *
 * Streaming over the radio can begin any time after cm32 has been
 * updated. For bandwidth efficiency, it's best to wait until after a
 * large update, then OR a single value with cm32 in order to trigger
 * the update.
 *
 * The system may still start streaming updates while the 'lock' bit
 * is set, but it will not be able to use the full range of protocol
 * optimization techniques, since some of these rely on knowing for
 * certain the values of words that have already been transmitted. So
 * it is recommended that userspace clear the 'lock' bits as soon as
 * possible after cm32 is set.
 */

struct _SYSVideoBuffer {
    union _SYSVideoRAM vram;
    uint32_t cm1[16];		/// INOUT  Change map, at a resolution of 1 bit per word
    uint32_t cm32;		/// INOUT  Change map, at a resolution of 1 bit per 32 words
    uint32_t lock;		/// OUT    Lock map, at a resolution of 1 bit per 16 words
};

/**
 * Accelerometer state
 */

struct _SYSAccelState {
    uint8_t x;
    uint8_t y;
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
    void (*assetDone)(_SYSCubeID cid);
    void (*accelChange)(_SYSCubeID cid);
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
void _SYS_paint(void);				/// Enqueue a new rendering frame

void _SYS_enableCubes(_SYSCubeIDVector cv);	/// Which cubes will be trying to connect?
void _SYS_disableCubes(_SYSCubeIDVector cv);

void _SYS_setVideoBuffer(_SYSCubeID cid, struct _SYSVideoBuffer *vbuf);
void _SYS_loadAssets(_SYSCubeID cid, struct _SYSAssetGroup *group);

void _SYS_getAccel(_SYSCubeID cid, struct _SYSAccelState *state);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
