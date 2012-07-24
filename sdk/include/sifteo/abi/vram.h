/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_ABI_VRAM_H
#define _SIFTEO_ABI_VRAM_H

#include <sifteo/abi/types.h>

#ifdef __cplusplus
extern "C" {
#endif

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

// Total size of VRAM, in bytes
#define _SYS_VRAM_BYTES         1024
#define _SYS_VRAM_BYTE_MASK     (_SYS_VRAM_BYTES - 1)

// Total size of VRAM, in words
#define _SYS_VRAM_WORDS         (_SYS_VRAM_BYTES / 2)
#define _SYS_VRAM_WORD_MASK     (_SYS_VRAM_WORDS - 1)

#define _SYS_VRAM_BG0_WIDTH     18      // Width/height of BG0 tile grid
#define _SYS_VRAM_BG1_WIDTH     16      // Width/height of BG1 bitmap
#define _SYS_VRAM_BG1_TILES     144     // Total number of opaque tiles in BG1
#define _SYS_VRAM_BG2_WIDTH     16      // Width/height of BG2 tile grid
#define _SYS_VRAM_SPRITES       8       // Total number of linear sprites
#define _SYS_SPRITES_PER_LINE   4       // Maximum visible sprites per scanline
#define _SYS_CHROMA_KEY         0x4f    // Chroma key MSB
#define _SYS_CKEY_BIT_EOL       0x40    // Chroma-key special bit, end-of-line

// Bits for 'flags'

#define _SYS_VF_RESERVED        0x01    // Reserved, must be zero
#define _SYS_VF_TOGGLE          0x02    // Toggle bit, to trigger a new frame render
#define _SYS_VF_SYNC            0x04    // Sync with LCD vertical refresh
#define _SYS_VF_CONTINUOUS      0x08    // Render continuously, without waiting for toggle
#define _SYS_VF_A21             0x10    // Flash A21 bank select
#define _SYS_VF_XY_SWAP         0x20    // Swap X and Y axes during render
#define _SYS_VF_X_FLIP          0x40    // Flip X axis during render
#define _SYS_VF_Y_FLIP          0x80    // Flip Y axis during render

// Values for 'mode'

#define _SYS_VM_MASK            0x3c    // Mask of valid bits in VM_MASK

#define _SYS_VM_POWERDOWN       0x00    // Power saving mode, LCD is off
#define _SYS_VM_BG0_ROM         0x04    // BG0, with tile data from internal ROM
#define _SYS_VM_SOLID           0x08    // Solid color, from colormap[0]
#define _SYS_VM_FB32            0x0c    // 32x32 pixel 16-color framebuffer
#define _SYS_VM_FB64            0x10    // 64x64 pixel 2-color framebuffer
#define _SYS_VM_FB128           0x14    // 128x48 pixel 2-color framebuffer
#define _SYS_VM_BG0             0x18    // Background BG0: 18x18 grid
#define _SYS_VM_BG0_BG1         0x1c    // BG0, plus overlay BG1: 16x16 bitmap + 144 indices
#define _SYS_VM_BG0_SPR_BG1     0x20    // BG0, multiple linear sprites, then BG1
#define _SYS_VM_BG2             0x24    // Background BG2: 16x16 grid with affine transform
#define _SYS_VM_STAMP           0x28    // Reconfigurable 16-color framebuffer with transparency
#define _SYS_VM_SLEEP           0x3c    // Puts cube to sleep after fading out display
    
// Important VRAM addresses

#define _SYS_VA_BG0_TILES       0x000
#define _SYS_VA_BG2_TILES       0x000
#define _SYS_VA_BG2_AFFINE      0x200
#define _SYS_VA_BG2_BORDER      0x20c
#define _SYS_VA_BG1_TILES       0x288
#define _SYS_VA_COLORMAP        0x300
#define _SYS_VA_BG1_BITMAP      0x3a8
#define _SYS_VA_SPR             0x3c8
#define _SYS_VA_BG1_XY          0x3f8
#define _SYS_VA_BG0_XY          0x3fa
#define _SYS_VA_FIRST_LINE      0x3fc
#define _SYS_VA_NUM_LINES       0x3fd
#define _SYS_VA_MODE            0x3fe
#define _SYS_VA_FLAGS           0x3ff
#define _SYS_VA_STAMP_PITCH     0x320

struct _SYSSpriteInfo {
    /*
     * Sprite sizes must be powers of two, and the
     * size/position must both be negated.
     *
     * Sprites can be disabled by setting height to zero.
     * A zero width with nonzero height will give undefined
     * results.
     */
    int8_t   mask_y;      // 0x00  (-height),  0 == disable
    int8_t   mask_x;      // 0x01  (-width)
    int8_t   pos_y;       // 0x02  (-top)
    int8_t   pos_x;       // 0x03  (-left)
    uint16_t  tile;       // 0x04  Address in 7:7 format
};

// Equivalent to an augmented matrix (3x2), in 8.8 fixed-point
struct _SYSAffine {
    int16_t cx;   // X initial value
    int16_t cy;   // Y initial value
    int16_t xx;   // X delta, for every horizontal pixel
    int16_t xy;   // Y delta, for every horizontal pixel
    int16_t yx;   // X delta, for every vertical pixel
    int16_t yy;   // Y delta, for every vertical pixel
};

union _SYSVideoRAM {
    uint8_t bytes[_SYS_VRAM_BYTES];
    uint16_t words[_SYS_VRAM_WORDS];

    struct {
        uint16_t bg0_tiles[324];        // 0x000 - 0x287
        uint16_t bg1_tiles[144];        // 0x288 - 0x3a7
        uint16_t bg1_bitmap[16];        // 0x3a8 - 0x3c7
        struct _SYSSpriteInfo spr[8];   // 0x3c8 - 0x3f7
        int8_t bg1_x;                   // 0x3f8
        int8_t bg1_y;                   // 0x3f9
        uint8_t bg0_x;                  // 0x3fa   0 <= x <= 143
        uint8_t bg0_y;                  // 0x3fb   0 <= y <= 143
        uint8_t first_line;             // 0x3fc   0 <= x <= 127
        uint8_t num_lines;              // 0x3fd   1 <= x <= 128
        uint8_t mode;                   // 0x3fe
        uint8_t flags;                  // 0x3ff
    };

    struct {
        uint8_t fb[768];                // 0x000 - 0x2ff
        uint16_t colormap[16];          // 0x300 - 0x31f
        uint8_t stamp_pitch;            // 0x320
        uint8_t stamp_height;           // 0x321
        uint8_t stamp_x;                // 0x322
        uint8_t stamp_width;            // 0x323
        uint8_t stamp_key;              // 0x324
    };

    struct {
        uint16_t bg2_tiles[256];        // 0x000 - 0x1ff
        struct _SYSAffine bg2_affine;   // 0x200 - 0x20b
        uint16_t bg2_border;            // 0x20c - 0x20d
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
 * Synchronization Protocol
 * ------------------------
 *
 * To support the system's asynchronous access requirements, these
 * buffers have a very strict synchronization protocol that user code
 * must observe:
 *
 *  1. Set the 'lock' bit(s) for any words you plan to update,
 *     using an atomic store.
 *
 *  2. Update the VRAM buffer, using any method and in any order you like.
 *
 *  3. Set change bits in cm1, using an atomic OR such as
 *     __builtin_or_and_fetch().
 *
 *  4. Set change bits in cm16, using an atomic OR. After this point the
 *     system may start sending data over the radio at any time.
 *
 *  5. Soon after step (4), clear the lock bits you set in (1). Since
 *     the system treats lock bits as read-only, you can do this using
 *     a plain atomic store.
 *
 * Note that cm16 is the primary trigger that the system uses in order
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
 * Streaming over the radio can begin any time after cm16 has been
 * updated. For bandwidth efficiency, it's best to wait until after a
 * large update, then OR a single value with cm16 in order to trigger
 * the update.
 *
 * The system may still start streaming updates while the 'lock' bit
 * is set, but it will not be able to use the full range of protocol
 * optimization techniques, since some of these rely on knowing for
 * certain the values of words that have already been transmitted. So
 * it is recommended that userspace clear the 'lock' bits as soon as
 * possible after cm16 is set.
 *
 * Asynchronous Pipeline
 * ---------------------
 *
 * When working with the graphics subsystem, it's helpful to keep in mind
 * the pipeline of asynchronous rendering stages:
 * 
 *   1. Changes are written to a locked vbuf (lock)
 *   2. The vbuf is unlocked, changes can start to stream over the air (cm16 <- lock)
 *   3. Streaming is finished (cm16 <- 0)
 *   4. The cube hardware begins rendering a frame
 *   5. The cube acknowledges rendering completion
 *
 * At any of these stages, a video buffer change can 'invalidate' the
 * ongoing rendering, meaning we may need to render an additional frame before
 * the final intended output is actually present on the display. Rendering is
 * only considered to be 'finished' if we make it through steps 3-5 without
 * any modifications to the video buffer.
 *
 * Paint Control
 * -------------
 *
 * The _SYS_paint() call is a no-op for cubes who have had no drawing occur.
 * This state is tracked, separately from the dirty bits, using a NEED_PAINT
 * flag. This flag is cleared by _SYS_paint() itself, whereas the actual dirty
 * bits are propagated through the firmware's model of the render pipeline.
 * Userspace code can force a cube to redraw by setting NEED_PAINT, even
 * without making any VRAM changes.
 *
 * This flag is set automatically by _SYS_vbuf_lock().
 */

#define _SYS_VBF_NEED_PAINT     (1 << 0)        // Request a paint operation
// All other bits are reserved for system use.

struct _SYSVideoBuffer {
    uint32_t flags;             /// INOUT  _SYS_VBF_* bits
    uint32_t lock;              /// OUT    Lock map, at a resolution of 1 bit per 16 words
    uint32_t cm16;              /// INOUT  Change map, at a resolution of 1 bit per 16 words
    uint32_t cm1[16];           /// INOUT  Change map, at a resolution of 1 bit per word
    union _SYSVideoRAM vram;
};

/*
 * In general, the system likes working with raw _SYSVideoBuffers: but in
 * userspace, it's very common to want to know both the _SYSVideoBuffer and
 * the ID of the cube it's currently attached with. For these cases, we
 * provide a standardized memory layout.
 */

struct _SYSAttachedVideoBuffer {
    _SYSCubeID cube;
    uint8_t reserved[3];
    struct _SYSVideoBuffer vbuf;
};

/*
 * Tiles in the _SYSVideoBuffer are typically encoded in 7:7 format, in
 * which a 14-bit tile ID is packed into the upper 7 bits of each byte
 * in a 16-bit word.
 */

#define _SYS_TILE77(_idx)   ((((_idx) << 2) & 0xFE00) | \
                             (((_idx) << 1) & 0x00FE))

#define _SYS_INVERSE_TILE77(_t77)   ((((_t77) & 0xFE00) >> 2) | \
                                     (((_t77) & 0x00FE) >> 1))

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
