/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
 * game code.
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

typedef uint8_t _SYSCubeID;             /// Cube slot index
typedef int8_t _SYSSideID;              /// Cube side index
typedef uint32_t _SYSCubeIDVector;      /// One bit for each cube slot, MSB-first

/**
 * Entry point. Our standard entry point is main(), with no arguments
 * or return values, declared using C linkage.
 *
 * The old name "siftmain" is supported for backward compatibility only.
 */

#define siftmain main

#ifdef __clang__     // Workaround for gcc's complaints about main() not returning int
void main(void);
#endif

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
    uint8_t hdrSize;            /// OUT    Size of header / offset to compressed data
    uint8_t reserved;           /// OUT    Reserved, must be zero
    uint16_t numTiles;          /// OUT    Uncompressed size, in tiles
    uint32_t dataSize;          /// OUT    Size of compressed data, in bytes
    uint64_t signature;         /// OUT    Unique identity for this group
};

struct _SYSAssetGroupCube {
    uint16_t baseAddr;          /// IN     Installed base address, in tiles
    uint16_t reserved;          /// IN     Must be zero
    uint32_t progress;          /// IN     Loading progress, in bytes
};

struct _SYSAssetGroup {
    uint32_t pHdr;                  /// OUT    Read-only data for this asset group
    uint32_t pCubes;                /// OUT    Array of per-cube state buffers
    _SYSCubeIDVector reqCubes;      /// IN     Which cubes have requested to load this group?
    _SYSCubeIDVector doneCubes;     /// IN     Which cubes have finished installing this group?
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

#define _SYS_VF_TOGGLE          0x02    // Toggle bit, to trigger a new frame render
#define _SYS_VF_SYNC            0x04    // Sync with LCD vertical refresh
#define _SYS_VF_CONTINUOUS      0x08    // Render continuously, without waiting for toggle
#define _SYS_VF_RESERVED        0x10
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

// Important VRAM addresses

#define _SYS_VA_BG0_TILES       0x000
#define _SYS_VA_BG2_TILES       0x000
#define _SYS_VA_BG2_AFFINE      0x200
#define _SYS_VA_BG2_BORDER      0x20c
#define _SYS_VA_BG1_TILES       0x288
#define _SYS_VA_COLORMAP        0x300
#define _SYS_VA_BG1_BITMAP      0x3a8
#define _SYS_VA_SPR             0x3c8
#define _SYS_VA_FIRST_LINE      0x3fc
#define _SYS_VA_NUM_LINES       0x3fd
#define _SYS_VA_MODE            0x3fe
#define _SYS_VA_FLAGS           0x3ff

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
 *
 * The 'cm32next' member is special: It is not used at all by the
 * system when updating cubes, but it's used as bidirectional storage
 * for some of the system calls that manipulate video buffers. These
 * system calls will update cm32next instead of cm32, leaving it up to
 * the user to choose when to make those changes visible to the system
 * by OR'ing cm32next into cm32.
 *
 * 'needPaint' OR'ed with cm32 at every unlock, and cleared only after
 * we schedule a hardware repaint operation on the cubes. Userspace
 * code doesn't need to worry about this value at all, assuming you
 * use the system-provided unlock primitive.
 */

struct _SYSVideoBuffer {
    union _SYSVideoRAM vram;
    uint32_t cm1[16];           /// INOUT  Change map, at a resolution of 1 bit per word
    uint32_t cm32;              /// INOUT  Change map, at a resolution of 1 bit per 32 words
    uint32_t lock;              /// OUT    Lock map, at a resolution of 1 bit per 16 words
    uint32_t cm32next;          /// INOUT  Next CM32 change map.
    uint32_t needPaint;         /// INOUT  Repaint trigger
};


typedef uint32_t _SYSAudioHandle;

// NOTE - _SYS_AUDIO_BUF_SIZE must be power of 2 for our current FIFO implementation,
// but must also accommodate a full frame's worth of speex data. If we go narrowband,
// that's 160 shorts so we can get away with 512 bytes. Wideband is 320 shorts
// so we need to kick up to 1024 bytes. kind of a lot :/
#define _SYS_AUDIO_BUF_SIZE             (512 * sizeof(int16_t))
#define _SYS_AUDIO_MAX_CHANNELS         8

/*
 * Types of audio supported by the system
 */
enum _SYSAudioType {
    _SYS_Speex = 0,
    _SYS_PCM = 1
};

enum _SYSAudioLoopType {
    LoopOnce = 0,
    LoopRepeat = 1
};

struct _SYSAudioModule {
    uint8_t type;           /// _SYSAudioType code
    uint8_t reserved0;      /// Reserved, must be zero
    uint16_t reserved1;     /// Reserved, must be zero
    uint32_t dataSize;      /// Size of compressed data, in bytes
    uint32_t pData;         /// Flash address for compressed data
};

struct _SYSAudioBuffer {
    uint16_t head;
    uint16_t tail;
    uint8_t buf[_SYS_AUDIO_BUF_SIZE];
};

/**
 * Accelerometer state.
 */

struct _SYSAccelState {
    int8_t x;       // +X towards the right
    int8_t y;       // +Y towards the bottom
};

struct _SYSNeighborState {
    _SYSCubeID sides[4];
};

/**
 * Accelerometer tilt state, where each axis has three values ( -1, 0, 1)
 */

typedef enum {
	_SYS_TILT_NEGATIVE,
	_SYS_TILT_NEUTRAL,
	_SYS_TILT_POSITIVE
} _SYS_TiltType;

struct _SYSTiltState {
    unsigned x		: 4;
    unsigned y		: 4;
};

typedef enum {
  NOT_SHAKING,
  SHAKING
} _SYSShakeState;

/**
 * Event vectors. These can be changed at runtime in order to handle
 * events within the game binary, via _SYS_setVector / _SYS_getVector.
 */

typedef void (*_SYSCubeEvent)(void *context, _SYSCubeID cid);
typedef void (*_SYSNeighborEvent)(void *context,
    _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1);

typedef enum {
    _SYS_NEIGHBOR_ADD = 0,
    _SYS_NEIGHBOR_REMOVE,
    _SYS_CUBE_FOUND,
    _SYS_CUBE_LOST,
    _SYS_CUBE_ASSETDONE,
    _SYS_CUBE_ACCELCHANGE,
    _SYS_CUBE_TOUCH,
    _SYS_CUBE_TILT,
    _SYS_CUBE_SHAKE,
    
    _SYS_NUM_VECTORS,   // Must be last
} _SYSVectorID;

#define _SYS_NEIGHBOR_EVENTS    ( Sifteo::Intrinsic::LZ(_SYS_NEIGHBOR_ADD) |\
                                  Sifteo::Intrinsic::LZ(_SYS_NEIGHBOR_REMOVE) )
#define _SYS_CUBE_EVENTS        ( Sifteo::Intrinsic::LZ(_SYS_CUBE_FOUND) |\
                                  Sifteo::Intrinsic::LZ(_SYS_CUBE_LOST) |\
                                  Sifteo::Intrinsic::LZ(_SYS_CUBE_ASSETDONE) |\
                                  Sifteo::Intrinsic::LZ(_SYS_CUBE_ACCELCHANGE) |\
                                  Sifteo::Intrinsic::LZ(_SYS_CUBE_TOUCH) |\
                                  Sifteo::Intrinsic::LZ(_SYS_CUBE_TILT) |\
                                  Sifteo::Intrinsic::LZ(_SYS_CUBE_SHAKE) )

/**
 * Internal state of the Pseudorandom Number Generator, maintained in user RAM.
 */

struct _SYSPseudoRandomState {
    uint32_t a, b, c, d;
};

/**
 * Hardware IDs are 48-bit / 6-byte numbers that uniquely identify a
 * particular cube. A valid HWIDs never contains 0xFF bytes.
 */

#define _SYS_HWID_BYTES         6
#define _SYS_HWID_BITS          48
#define _SYS_INVALID_HWID       ((uint64_t)-1)

/**
 * Binary metadata.
 *
 * Loadable programs in this system are standard ELF binaries, however their
 * instruction set is a special restricted subset of Thumb-2 as defined by the
 * Sifteo Virtual Machine.
 *
 * In addition to standard read-only data, read-write data, and BSS segments,
 * we support a special metadata segment. This contains a key-value dictionary
 * of metadata records.
 *
 * The contents of the metadata segment is structured as first an array of
 * key/size words, then a stream of variable-size values. The values must be
 * aligned according to their natural ABI alignment, and they must not cross
 * a memory page boundary.
 *
 * Since the p_paddr field in the phdr is typically unused, we overload that
 * to store the number of keys in the array.
 *
 * Strings are zero-terminated. Additional padding bytes may appear after
 * any value.
 */

#define _SYS_ELF_PT_METADATA        0x7f7c0000      // Metadata phdr type

struct _SYSMetadataKey {
    uint16_t    stride;             // Byte offset from this value to the next
    uint16_t    type;               // _SYS_METADATA_*
};

/// Metadata types
#define _SYS_METADATA_NONE          0x0000  // Ignored. (padding)
#define _SYS_METADATA_UUID          0x0001  // Binary UUID for this specific build
#define _SYS_METADATA_AGSLOT        0x0002  // Array of _SYSAssetGroupSlotMetadata
#define _SYS_METADATA_TITLE_STR     0x0003  // Human readable game title string
#define _SYS_METADATA_PACKAGE_STR   0x0004  // DNS-style package string
#define _STS_METADATA_VERSION_STR   0x0005  // Version string
#define _SYS_METADATA_ICON_80x80    0x0006  // _SYSMetadataPinnedImage

struct _SYSMetadataPinnedImage {
    uint32_t    groupHdr;       // File offset for _SYSAssetGroupHeader    
    uint16_t    index;          // First tile index in image
    uint16_t    reserved;       // Must be zero
};

/**
 * Link-time intrinsics.
 *
 * These functions are replaced during link-time optimization.
 *
 * Logging supports many standard printf() format specifiers:
 *   - Literal characters, and %%
 *   - Standard integer specifiers: %d, %i, %o, %u, %X, %x, %p, %c
 *   - Standard float specifiers: %f, %F, %e, %E, %g, %G
 *   - Four chars packed into a 32-bit integer: %C
 *   - Binary integers: %b
 *   - C-style strings: %s
 *   - Hex-dump of fixed width buffers: %<width>h
 */

unsigned _SYS_lti_isDebug();
void _SYS_lti_log(const char *fmt, ...);
void _SYS_lti_metadata_str(uint16_t type, const char *str);

/**
 * Type bits, for use in the 'tag' for the low-level _SYS_log() handler.
 * Normally these don't need to be used in usermode code, they're inserted
 * automatically by slinky when expanding _SYS_lti_log().
 */

#define _SYS_LOGTYPE_FMT        0       // param = strtab offest
#define _SYS_LOGTYPE_STRING     1       // param = 0, v1 = ptr
#define _SYS_LOGTYPE_HEXDUMP    2       // param = length, v1 = ptr

/**
 * Low-level system call interface.
 *
 * System calls #0-63 are faster and smaller than normal function calls,
 * whereas all other syscalls (#64-8191) are similar in cost to a normal
 * call. System calls use a simplified calling convention that supports
 * only at most 8 32-bit integer parameters, with at most one (32/64-bit)
 * integer result.
 */

#if defined(FW_BUILD) || !defined(__clang__)
#  define _SC(n)
#  define _NORET
#else
#  define _SC(n)    __asm__ ("_SYS_" #n)
#  define _NORET    __attribute__ ((noreturn))
#endif

void _SYS_abort(void) _SC(0) _NORET;
void _SYS_exit(void) _SC(74) _NORET;

void _SYS_yield(void) _SC(75);   /// Temporarily cede control to the firmware
void _SYS_paint(void) _SC(76);   /// Enqueue a new rendering frame
void _SYS_finish(void) _SC(77);  /// Wait for enqueued frames to finish

// Lightweight event logging support: string identifier plus 0-7 integers.
// Tag bits: type [31:27], arity [26:24] param [23:0]
void _SYS_log(uint32_t tag, uintptr_t v1, uintptr_t v2, uintptr_t v3, uintptr_t v4, uintptr_t v5, uintptr_t v6, uintptr_t v7) _SC(25);

// Compiler floating point support
uint32_t _SYS_add_f32(uint32_t a, uint32_t b) _SC(63);
uint64_t _SYS_add_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(62);
uint32_t _SYS_sub_f32(uint32_t a, uint32_t b) _SC(61);
uint64_t _SYS_sub_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(60);
uint32_t _SYS_mul_f32(uint32_t a, uint32_t b) _SC(59);
uint64_t _SYS_mul_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(58);
uint32_t _SYS_div_f32(uint32_t a, uint32_t b) _SC(57);
uint64_t _SYS_div_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(56);
uint64_t _SYS_fpext_f32_f64(uint32_t a) _SC(55);
uint32_t _SYS_fpround_f64_f32(uint32_t aL, uint32_t aH) _SC(54);
uint32_t _SYS_fptosint_f32_i32(uint32_t a) _SC(53);
uint64_t _SYS_fptosint_f32_i64(uint32_t a) _SC(52);
uint32_t _SYS_fptosint_f64_i32(uint32_t aL, uint32_t aH) _SC(51);
uint64_t _SYS_fptosint_f64_i64(uint32_t aL, uint32_t aH) _SC(50);
uint32_t _SYS_fptouint_f32_i32(uint32_t a) _SC(49);
uint64_t _SYS_fptouint_f32_i64(uint32_t a) _SC(48);
uint32_t _SYS_fptouint_f64_i32(uint32_t aL, uint32_t aH) _SC(47);
uint64_t _SYS_fptouint_f64_i64(uint32_t aL, uint32_t aH) _SC(46);
uint32_t _SYS_sinttofp_i32_f32(uint32_t a) _SC(45);
uint64_t _SYS_sinttofp_i32_f64(uint32_t a) _SC(44);
uint32_t _SYS_sinttofp_i64_f32(uint32_t aL, uint32_t aH) _SC(43);
uint64_t _SYS_sinttofp_i64_f64(uint32_t aL, uint32_t aH) _SC(42);
uint32_t _SYS_uinttofp_i32_f32(uint32_t a) _SC(41);
uint64_t _SYS_uinttofp_i32_f64(uint32_t a) _SC(40);
uint32_t _SYS_uinttofp_i64_f32(uint32_t aL, uint32_t aH) _SC(39);
uint64_t _SYS_uinttofp_i64_f64(uint32_t aL, uint32_t aH) _SC(38);
uint32_t _SYS_eq_f32(uint32_t a, uint32_t b) _SC(37);
uint32_t _SYS_eq_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(36);
uint32_t _SYS_lt_f32(uint32_t a, uint32_t b) _SC(35);
uint32_t _SYS_lt_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(34);
uint32_t _SYS_le_f32(uint32_t a, uint32_t b) _SC(33);
uint32_t _SYS_le_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(32);
uint32_t _SYS_ge_f32(uint32_t a, uint32_t b) _SC(31);
uint32_t _SYS_ge_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(30);
uint32_t _SYS_gt_f32(uint32_t a, uint32_t b) _SC(29);
uint32_t _SYS_gt_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(28);
uint32_t _SYS_un_f32(uint32_t a, uint32_t b) _SC(27);
uint32_t _SYS_un_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(26);

// Compiler atomics support
uint32_t _SYS_fetch_and_or_4(uint32_t *p, uint32_t t) _SC(21);
uint32_t _SYS_fetch_and_xor_4(uint32_t *p, uint32_t t) _SC(22);
uint32_t _SYS_fetch_and_nand_4(uint32_t *p, uint32_t t) _SC(23);
uint32_t _SYS_fetch_and_and_4(uint32_t *p, uint32_t t) _SC(24);

// Compiler support for 64-bit operations
uint64_t _SYS_shl_i64(uint32_t aL, uint32_t aH, uint32_t b) _SC(107);
uint64_t _SYS_srl_i64(uint32_t aL, uint32_t aH, uint32_t b) _SC(108);
int64_t _SYS_sra_i64(uint32_t aL, uint32_t aH, uint32_t b) _SC(109);

void _SYS_sincosf(uint32_t x, float *sinOut, float *cosOut) _SC(8);
uint32_t _SYS_fmodf(uint32_t a, uint32_t b) _SC(9);

void _SYS_memset8(uint8_t *dest, uint8_t value, uint32_t count) _SC(1);
void _SYS_memset16(uint16_t *dest, uint16_t value, uint32_t count) _SC(2);
void _SYS_memset32(uint32_t *dest, uint32_t value, uint32_t count) _SC(3);
void _SYS_memcpy8(uint8_t *dest, const uint8_t *src, uint32_t count) _SC(4);
void _SYS_memcpy16(uint16_t *dest, const uint16_t *src, uint32_t count) _SC(5);
void _SYS_memcpy32(uint32_t *dest, const uint32_t *src, uint32_t count) _SC(6);
int _SYS_memcmp8(const uint8_t *a, const uint8_t *b, uint32_t count) _SC(7);

uint32_t _SYS_strnlen(const char *str, uint32_t maxLen) _SC(17);
void _SYS_strlcpy(char *dest, const char *src, uint32_t destSize) _SC(18);
void _SYS_strlcat(char *dest, const char *src, uint32_t destSize) _SC(19);
void _SYS_strlcat_int(char *dest, int src, uint32_t destSize) _SC(20);
void _SYS_strlcat_int_fixed(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize) _SC(68);
void _SYS_strlcat_int_hex(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize) _SC(69);
int _SYS_strncmp(const char *a, const char *b, uint32_t count) _SC(70);

void _SYS_prng_init(struct _SYSPseudoRandomState *state, uint32_t seed) _SC(71);
uint32_t _SYS_prng_value(struct _SYSPseudoRandomState *state) _SC(10);
uint32_t _SYS_prng_valueBounded(struct _SYSPseudoRandomState *state, uint32_t limit) _SC(11);

int64_t _SYS_ticks_ns(void) _SC(12);  /// Return the monotonic system timer, in 64-bit integer nanoseconds

void _SYS_setVector(_SYSVectorID vid, void *handler, void *context) _SC(104);
void *_SYS_getVectorHandler(_SYSVectorID vid) _SC(105);
void *_SYS_getVectorContext(_SYSVectorID vid) _SC(106);

void _SYS_solicitCubes(_SYSCubeID min, _SYSCubeID max) _SC(79);
void _SYS_enableCubes(_SYSCubeIDVector cv) _SC(80);  /// Which cubes will be trying to connect?
void _SYS_disableCubes(_SYSCubeIDVector cv) _SC(81);

void _SYS_setVideoBuffer(_SYSCubeID cid, struct _SYSVideoBuffer *vbuf) _SC(82);
void _SYS_loadAssets(_SYSCubeID cid, struct _SYSAssetGroup *group) _SC(83);

struct _SYSAccelState _SYS_getAccel(_SYSCubeID cid) _SC(84);
void _SYS_getNeighbors(_SYSCubeID cid, struct _SYSNeighborState *state) _SC(85);
struct _SYSTiltState _SYS_getTilt(_SYSCubeID cid) _SC(86);
_SYSShakeState _SYS_getShake(_SYSCubeID cid) _SC(87);

// XXX: Temporary for testing/demoing
uint16_t _SYS_getRawBatteryV(_SYSCubeID cid) _SC(88);

uint8_t _SYS_isTouching(_SYSCubeID cid) _SC(90);
uint64_t _SYS_getCubeHWID(_SYSCubeID cid) _SC(91);

void _SYS_vbuf_init(struct _SYSVideoBuffer *vbuf) _SC(92);
void _SYS_vbuf_lock(struct _SYSVideoBuffer *vbuf, uint16_t addr) _SC(93);
void _SYS_vbuf_unlock(struct _SYSVideoBuffer *vbuf) _SC(94);
void _SYS_vbuf_poke(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word) _SC(13);
void _SYS_vbuf_pokeb(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint8_t byte) _SC(14);
uint16_t _SYS_vbuf_peek(const struct _SYSVideoBuffer *vbuf, uint16_t addr) _SC(15);
uint8_t _SYS_vbuf_peekb(const struct _SYSVideoBuffer *vbuf, uint16_t addr) _SC(16);
void _SYS_vbuf_fill(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word, uint16_t count) _SC(99);
void _SYS_vbuf_seqi(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t index, uint16_t count) _SC(100);
void _SYS_vbuf_write(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t count) _SC(101);
void _SYS_vbuf_writei(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count) _SC(102);
void _SYS_vbuf_wrect(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count, uint16_t lines, uint16_t src_stride, uint16_t addr_stride) _SC(103);
void _SYS_vbuf_spr_resize(struct _SYSVideoBuffer *vbuf, unsigned id, unsigned width, unsigned height) _SC(98);
void _SYS_vbuf_spr_move(struct _SYSVideoBuffer *vbuf, unsigned id, int x, int y) _SC(97);

void _SYS_audio_enableChannel(struct _SYSAudioBuffer *buffer) _SC(96);
uint8_t _SYS_audio_play(const struct _SYSAudioModule *mod, _SYSAudioHandle *h, enum _SYSAudioLoopType loop) _SC(95);
uint8_t _SYS_audio_isPlaying(_SYSAudioHandle h) _SC(78);
void _SYS_audio_stop(_SYSAudioHandle h) _SC(73);
void _SYS_audio_pause(_SYSAudioHandle h) _SC(72);
void _SYS_audio_resume(_SYSAudioHandle h) _SC(67);
int  _SYS_audio_volume(_SYSAudioHandle h) _SC(66);
void _SYS_audio_setVolume(_SYSAudioHandle h, int volume) _SC(65);
uint32_t _SYS_audio_pos(_SYSAudioHandle h) _SC(64);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
