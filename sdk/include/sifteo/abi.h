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
#else
typedef uint8_t bool;
#endif

/**
 * Data types which are valid across the user/system boundary.
 *
 * Data directions are from the userspace's perspective. IN is
 * firmware -> game, OUT is game -> firmware.
 */

#define _SYS_NUM_CUBE_SLOTS     32
#define _SYS_CUBE_ID_INVALID    0xFF    /// Reserved _SYSCubeID value

typedef uint8_t _SYSCubeID;             /// Cube slot index
typedef int8_t _SYSSideID;              /// Cube side index
typedef uint32_t _SYSCubeIDVector;      /// One bit for each cube slot, MSB-first
typedef uint8_t _SYSAssetSlot;          /// Ordinal for one of the game's asset slots

/**
 * Entry point. Our standard entry point is main(), with no arguments
 * or return values, declared using C linkage.
 */

#ifdef __clang__   // Workaround for gcc's complaints about main() not returning int
void main(void);
#endif

#define _SYS_ASSETLOAD_BUF_SIZE  48   // Makes _SYSAssetLoaderCube come to 64 bytes

struct _SYSAssetGroupHeader {
    uint8_t reserved;           /// OUT    Reserved, must be zero
    uint8_t ordinal;            /// OUT    Small integer, unique within an ELF
    uint16_t numTiles;          /// OUT    Uncompressed size, in tiles
    uint32_t dataSize;          /// OUT    Size of compressed data, in bytes
    uint64_t hash;              /// OUT    Hash of this asset group's data
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
    uint32_t pAssetGroup;   /// IN    Address for _SYSAssetGroup in RAM
    uint32_t progress;      /// IN    Number of compressed bytes read from flash
    uint32_t dataSize;      /// IN    Local copy of asset group's dataSize
    uint16_t reserved;      /// -
    uint8_t head;           /// -     Index of the next sample to read
    uint8_t tail;           /// -     Index of the next empty slot to write into
    uint8_t buf[_SYS_ASSETLOAD_BUF_SIZE];
};

struct _SYSAssetLoader {
    _SYSCubeIDVector cubeVec;   /// OUT   Which _SYSAssetLoaderCube structs are valid?
    _SYSCubeIDVector complete;  /// OUT   Which cubes have fully completed their loading?
    // Followed by a _SYSAssetLoaderCube array
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

/**
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

/**
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

/**
 * Tiles in the _SYSVideoBuffer are typically encoded in 7:7 format, in
 * which a 14-bit tile ID is packed into the upper 7 bits of each byte
 * in a 16-bit word.
 */

#define _SYS_TILE77(_idx)   ((((_idx) << 2) & 0xFE00) | \
                             (((_idx) << 1) & 0x00FE))

/*
 * Audio handles
 */

#define _SYS_AUDIO_INVALID_HANDLE   ((uint32_t)-1)

typedef uint32_t _SYSAudioHandle;

// NOTE - _SYS_AUDIO_BUF_SIZE must be power of 2 for our current FIFO implementation,
// but must also accommodate a full frame's worth of speex data. If we go narrowband,
// that's 160 shorts so we can get away with 512 bytes. Wideband is 320 shorts
// so we need to kick up to 1024 bytes. kind of a lot :/
#define _SYS_AUDIO_BUF_SIZE             (512 * sizeof(int16_t))
#define _SYS_AUDIO_MAX_CHANNELS         8

#define _SYS_AUDIO_MAX_VOLUME       256   // Guaranteed to be a power of two
#define _SYS_AUDIO_DEFAULT_VOLUME   128

/*
 * Types of audio supported by the system
 */
enum _SYSAudioType {
    _SYS_PCM = 1,
    _SYS_ADPCM = 2
};

enum _SYSAudioLoopType {
    _SYS_LOOP_UNDEF     = -1,
    _SYS_LOOP_ONCE      = 0,
    _SYS_LOOP_REPEAT    = 1,
    _SYS_LOOP_PING_PONG = 2
};

struct _SYSAudioModule {
    uint32_t sampleRate;    /// Native sampling rate of data
    uint32_t loopStart;     /// Loop starting point, in samples
    uint32_t loopEnd;       /// Loop ending point, in samples
    uint8_t loopType;       /// Loop type, 0 (no looping) or 1 (forward loop)
    uint8_t type;           /// _SYSAudioType code
    uint16_t volume;        /// Sample default volume (overridden by explicit channel volume)
    uint32_t dataSize;      /// Size of compressed data, in bytes
    uint32_t pData;         /// Flash address for compressed data
};

struct _SYSAudioBuffer {
    uint16_t head;          /// Index of the next sample to read
    uint16_t tail;          /// Index of the next empty slot to write into
    uint8_t buf[_SYS_AUDIO_BUF_SIZE];
};


/**
 * Small vector types
 */

struct _SYSInt2 {
    int32_t x, y;
};

union _SYSByte4 {
    uint32_t value;
    struct {
        int8_t x, y, z, w;
    };
};

union _SYSNeighborState {
    uint32_t value;
    _SYSCubeID sides[4];
};


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

#define _SYS_NEIGHBOR_EVENTS    ( (0x80000000 >> _SYS_NEIGHBOR_ADD) |\
                                  (0x80000000 >> _SYS_NEIGHBOR_REMOVE) )
#define _SYS_CUBE_EVENTS        ( (0x80000000 >> _SYS_CUBE_FOUND) |\
                                  (0x80000000 >> _SYS_CUBE_LOST) |\
                                  (0x80000000 >> _SYS_CUBE_ASSETDONE) |\
                                  (0x80000000 >> _SYS_CUBE_ACCELCHANGE) |\
                                  (0x80000000 >> _SYS_CUBE_TOUCH) |\
                                  (0x80000000 >> _SYS_CUBE_TILT) |\
                                  (0x80000000 >> _SYS_CUBE_SHAKE) )

/**
 * Internal state of the Pseudorandom Number Generator, maintained in user RAM.
 */

struct _SYSPseudoRandomState {
    uint32_t a, b, c, d;
};

/**
 * Hardware IDs are 64-bit numbers that uniquely identify a
 * particular cube. A valid HWIDs never contains 0xFF bytes.
 */

#define _SYS_HWID_BYTES         8
#define _SYS_HWID_BITS          64
#define _SYS_INVALID_HWID       ((uint64_t)-1)

/**
 * RFC4122 compatible UUIDs.
 *
 * These are used in game metadata, to uniquely identify a particular
 * binary. They are stored in network byte order, with field names compatible
 * with RFC4122.
 */

struct _SYSUUID {
    union {
        struct {
            uint32_t time_low;
            uint16_t time_mid;
            uint16_t time_hi_and_version;
            uint8_t clk_seq_hi_res;
            uint8_t clk_seq_low;
            uint8_t node[6];
        };
        uint8_t  bytes[16];
        uint16_t hwords[8];
        uint32_t words[4];
    };
};

/**
 * ELF binary format.
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
 * a memory page boundary. Each key occurs at most once in this table; multiple
 * values with the same key are concatenated by the linker.
 *
 * The last _SYSMetadataKey has the MSB set in its 'stride' value.
 *
 * Strings are zero-terminated. Additional padding bytes may appear after
 * any value.
 */

/// SVM-specific program header types
#define _SYS_ELF_PT_METADATA        0x7000f001
#define _SYS_ELF_PT_LOAD_RLE        0x7000f002

struct _SYSMetadataKey {
    uint16_t    stride;     // Byte offset from this value to the next
    uint16_t    key;        // _SYS_METADATA_*
};

/// Metadata keys
#define _SYS_METADATA_NONE          0x0000  // Ignored. (padding)
#define _SYS_METADATA_UUID          0x0001  // Binary UUID for this specific build
#define _SYS_METADATA_BOOT_ASSET    0x0002  // Array of _SYSMetadataBootAsset
#define _SYS_METADATA_TITLE_STR     0x0003  // Human readable game title string
#define _SYS_METADATA_PACKAGE_STR   0x0004  // DNS-style package string
#define _STS_METADATA_VERSION_STR   0x0005  // Version string
#define _SYS_METADATA_ICON_80x80    0x0006  // _SYSMetadataImage
#define _SYS_METADATA_NUM_ASLOTS    0x0007  // uint8_t, count of required AssetSlots
#define _SYS_METADATA_CUBE_RANGE    0x0008  // _SYSMetadataCubeRange

struct _SYSMetadataBootAsset {
    uint32_t        pHdr;           // Virtual address for _SYSAssetGroupHeader
    _SYSAssetSlot   slot;           // Asset group slot to load this into
    uint8_t         reserved[3];    // Must be zero;
};

struct _SYSMetadataCubeRange {
    uint8_t     minCubes;
    uint8_t     maxCubes;
};

struct _SYSMetadataImage {
    uint8_t   width;            /// Width of the image, in tiles
    uint8_t   height;           /// Height of the image, in tiles
    uint8_t   frames;           /// Number of "frames" in this image
    uint8_t   format;           /// _SYSAssetImageFormat
    uint32_t  groupHdr;         /// Virtual address for _SYSAssetGroupHeader
    uint32_t  pData;            /// Format-specific data or data pointer
};

/**
 * Link-time intrinsics.
 *
 * These functions are replaced during link-time optimization.
 *
 * Logging supports many standard printf() format specifiers:
 *
 *   - Literal characters, and %%
 *   - Standard integer specifiers: %d, %i, %o, %u, %X, %x, %p, %c
 *   - Standard float specifiers: %f, %F, %e, %E, %g, %G
 *   - Four chars packed into a 32-bit integer: %C
 *   - Binary integers: %b
 *   - C-style strings: %s
 *   - Hex-dump of fixed width buffers: %<width>h
 *   - Pointer, printed as a resolved symbol when possible: %P
 *
 * To work around limitations in C variadic functions, _SYS_lti_metadata()
 * supports a format string which specifies what data type each argument
 * should be cast to. Data types here automatically imply ABI-compatible
 * alignment and padding:
 *
 *   "b" = int8_t
 *   "B" = uint8_t
 *   "h" = int16_t
 *   "H" = uint16_t
 *   "i" = int32_t
 *   "I" = uint32_t
 *   "s" = String (NUL terminator is *not* automatically added)
 *
 * Counters:
 *   This is a mechanism for generating monotonic unique IDs at link-time.
 *   Every _SYS_lti_counter() call with the same 'name' will return a
 *   different value, starting with zero. Values are assigned in order of
 *   decreasing priority.
 *
 * UUIDs:
 *   We support link-time generation of standard UUIDs. For every unique
 *   'key', the linker will generate a different UUID. Since a full UUID
 *   is too large to return directly, it is accessed as a group of four
 *   little-endian 32-bit words, using values of 'index' from 0 to 3.
 *
 * Static initializers:
 *   In global varaibles which aren't themselves constant but which were
 *   initialized to a constant, _SYS_lti_initializer() can be used to retrieve
 *   that initializer value at link-time. If 'require' is 'true', the value
 *   must be resolveable to a constant static initializer of a link error
 *   will result. If 'require' is false, we return the static initializer if
 *   possible, or pass through 'value' without modification if not.
 */

unsigned _SYS_lti_isDebug();
void _SYS_lti_abort(bool enable, const char *message);
void _SYS_lti_log(const char *fmt, ...);
void _SYS_lti_metadata(uint16_t key, const char *fmt, ...);
unsigned _SYS_lti_counter(const char *name, int priority);
uint32_t _SYS_lti_uuid(unsigned key, unsigned index);
const void *_SYS_lti_initializer(const void *value, bool require);
bool _SYS_lti_isConstant(unsigned value);

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
 * call.
 *
 * System calls use a simplified calling convention that supports
 * only at most 8 32-bit integer parameters, with at most one (32/64-bit)
 * integer result.
 *
 * Parameters are allowed to be arbitrary-width integers, but they must
 * not be floats. Any function that takes float parameters must explicitly
 * bitcast them to integers.
 *
 * Return values must be integers, and furthermore they must be exactly
 * 32 or 64 bits wide.
 */

#if defined(FW_BUILD) || !defined(__clang__)
#  define _SC(n)
#  define _NORET
#else
#  define _SC(n)    __asm__ ("_SYS_" #n)
#  define _NORET    __attribute__ ((noreturn))
#endif

void _SYS_abort(void) _SC(0) _NORET;
void _SYS_exit(void) _SC(64) _NORET;

uint32_t _SYS_getFeatures() _SC(145);   /// ABI compatibility feature bits

void _SYS_yield(void) _SC(65);   /// Temporarily cede control to the firmware
void _SYS_paint(void) _SC(66);   /// Enqueue a new rendering frame
void _SYS_finish(void) _SC(67);  /// Wait for enqueued frames to finish

// Lightweight event logging support: string identifier plus 0-7 integers.
// Tag bits: type [31:27], arity [26:24] param [23:0]
void _SYS_log(uint32_t tag, uintptr_t v1, uintptr_t v2, uintptr_t v3, uintptr_t v4, uintptr_t v5, uintptr_t v6, uintptr_t v7) _SC(17);

// Compiler floating point support
uint32_t _SYS_add_f32(uint32_t a, uint32_t b) _SC(22);
uint64_t _SYS_add_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(68);
uint32_t _SYS_sub_f32(uint32_t a, uint32_t b) _SC(30);
uint64_t _SYS_sub_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(69);
uint32_t _SYS_mul_f32(uint32_t a, uint32_t b) _SC(25);
uint64_t _SYS_mul_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(34);
uint32_t _SYS_div_f32(uint32_t a, uint32_t b) _SC(37);
uint64_t _SYS_div_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(70);
uint64_t _SYS_fpext_f32_f64(uint32_t a) _SC(71);
uint32_t _SYS_fpround_f64_f32(uint32_t aL, uint32_t aH) _SC(36);
uint32_t _SYS_fptosint_f32_i32(uint32_t a) _SC(31);
uint64_t _SYS_fptosint_f32_i64(uint32_t a) _SC(72);
uint32_t _SYS_fptosint_f64_i32(uint32_t aL, uint32_t aH) _SC(73);
uint64_t _SYS_fptosint_f64_i64(uint32_t aL, uint32_t aH) _SC(74);
uint32_t _SYS_fptouint_f32_i32(uint32_t a) _SC(75);
uint64_t _SYS_fptouint_f32_i64(uint32_t a) _SC(76);
uint32_t _SYS_fptouint_f64_i32(uint32_t aL, uint32_t aH) _SC(77);
uint64_t _SYS_fptouint_f64_i64(uint32_t aL, uint32_t aH) _SC(78);
uint32_t _SYS_sinttofp_i32_f32(uint32_t a) _SC(49);
uint64_t _SYS_sinttofp_i32_f64(uint32_t a) _SC(39);
uint32_t _SYS_sinttofp_i64_f32(uint32_t aL, uint32_t aH) _SC(79);
uint64_t _SYS_sinttofp_i64_f64(uint32_t aL, uint32_t aH) _SC(80);
uint32_t _SYS_uinttofp_i32_f32(uint32_t a) _SC(45);
uint64_t _SYS_uinttofp_i32_f64(uint32_t a) _SC(81);
uint32_t _SYS_uinttofp_i64_f32(uint32_t aL, uint32_t aH) _SC(82);
uint64_t _SYS_uinttofp_i64_f64(uint32_t aL, uint32_t aH) _SC(83);
uint32_t _SYS_eq_f32(uint32_t a, uint32_t b) _SC(84);
uint32_t _SYS_eq_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(85);
uint32_t _SYS_lt_f32(uint32_t a, uint32_t b) _SC(44);
uint32_t _SYS_lt_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(86);
uint32_t _SYS_le_f32(uint32_t a, uint32_t b) _SC(40);
uint32_t _SYS_le_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(87);
uint32_t _SYS_ge_f32(uint32_t a, uint32_t b) _SC(32);
uint32_t _SYS_ge_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(88);
uint32_t _SYS_gt_f32(uint32_t a, uint32_t b) _SC(42);
uint32_t _SYS_gt_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(89);
uint32_t _SYS_un_f32(uint32_t a, uint32_t b) _SC(27);
uint32_t _SYS_un_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(90);

// Compiler atomics support
uint32_t _SYS_fetch_and_or_4(uint32_t *p, uint32_t t) _SC(91);
uint32_t _SYS_fetch_and_xor_4(uint32_t *p, uint32_t t) _SC(92);
uint32_t _SYS_fetch_and_nand_4(uint32_t *p, uint32_t t) _SC(93);
uint32_t _SYS_fetch_and_and_4(uint32_t *p, uint32_t t) _SC(94);

// Compiler support for 64-bit operations
uint64_t _SYS_shl_i64(uint32_t aL, uint32_t aH, uint32_t b) _SC(95);
uint64_t _SYS_srl_i64(uint32_t aL, uint32_t aH, uint32_t b) _SC(96);
int64_t _SYS_sra_i64(uint32_t aL, uint32_t aH, uint32_t b) _SC(97);
uint64_t _SYS_mul_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(43);
int64_t _SYS_sdiv_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(29);
uint64_t _SYS_udiv_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(98);
int64_t _SYS_srem_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(99);
uint64_t _SYS_urem_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(100);

void _SYS_sincosf(uint32_t x, float *sinOut, float *cosOut) _SC(56);
uint32_t _SYS_fmodf(uint32_t a, uint32_t b) _SC(53);
uint32_t _SYS_sqrtf(uint32_t a) _SC(101);
uint64_t _SYS_fmod(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(102);
uint64_t _SYS_sqrt(uint32_t aL, uint32_t aH) _SC(103);

void _SYS_memset8(uint8_t *dest, uint8_t value, uint32_t count) _SC(62);
void _SYS_memset16(uint16_t *dest, uint16_t value, uint32_t count) _SC(26);
void _SYS_memset32(uint32_t *dest, uint32_t value, uint32_t count) _SC(104);
void _SYS_memcpy8(uint8_t *dest, const uint8_t *src, uint32_t count) _SC(105);
void _SYS_memcpy16(uint16_t *dest, const uint16_t *src, uint32_t count) _SC(48);
void _SYS_memcpy32(uint32_t *dest, const uint32_t *src, uint32_t count) _SC(106);
int32_t _SYS_memcmp8(const uint8_t *a, const uint8_t *b, uint32_t count) _SC(107);

uint32_t _SYS_strnlen(const char *str, uint32_t maxLen) _SC(108);
void _SYS_strlcpy(char *dest, const char *src, uint32_t destSize) _SC(57);
void _SYS_strlcat(char *dest, const char *src, uint32_t destSize) _SC(41);
void _SYS_strlcat_int(char *dest, int src, uint32_t destSize) _SC(58);
void _SYS_strlcat_int_fixed(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize) _SC(109);
void _SYS_strlcat_int_hex(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize) _SC(110);
int32_t _SYS_strncmp(const char *a, const char *b, uint32_t count) _SC(111);

void _SYS_prng_init(struct _SYSPseudoRandomState *state, uint32_t seed) _SC(112);
uint32_t _SYS_prng_value(struct _SYSPseudoRandomState *state) _SC(38);
uint32_t _SYS_prng_valueBounded(struct _SYSPseudoRandomState *state, uint32_t limit) _SC(28);

int64_t _SYS_ticks_ns(void) _SC(23);  /// Return the monotonic system timer, in 64-bit integer nanoseconds

void _SYS_setVector(_SYSVectorID vid, void *handler, void *context) _SC(46);
void *_SYS_getVectorHandler(_SYSVectorID vid) _SC(113);
void *_SYS_getVectorContext(_SYSVectorID vid) _SC(114);

// Typically only needed by system menu code
void _SYS_enableCubes(_SYSCubeIDVector cv) _SC(59);
void _SYS_disableCubes(_SYSCubeIDVector cv) _SC(116);

void _SYS_setVideoBuffer(_SYSCubeID cid, struct _SYSVideoBuffer *vbuf) _SC(60);

uint32_t _SYS_getAccel(_SYSCubeID cid) _SC(117);
uint32_t _SYS_getNeighbors(_SYSCubeID cid) _SC(33);
uint32_t _SYS_getTilt(_SYSCubeID cid) _SC(61);
uint32_t _SYS_getShake(_SYSCubeID cid) _SC(118);

uint32_t _SYS_getBatteryV(_SYSCubeID cid) _SC(119);
uint32_t _SYS_isTouching(_SYSCubeID cid) _SC(51);
uint64_t _SYS_getCubeHWID(_SYSCubeID cid) _SC(120);

void _SYS_vbuf_init(struct _SYSVideoBuffer *vbuf) _SC(55);
void _SYS_vbuf_lock(struct _SYSVideoBuffer *vbuf, uint16_t addr) _SC(121);
void _SYS_vbuf_unlock(struct _SYSVideoBuffer *vbuf) _SC(122);
void _SYS_vbuf_poke(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word) _SC(18);
void _SYS_vbuf_pokeb(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint8_t byte) _SC(24);
uint32_t _SYS_vbuf_peek(const struct _SYSVideoBuffer *vbuf, uint16_t addr) _SC(123);
uint32_t _SYS_vbuf_peekb(const struct _SYSVideoBuffer *vbuf, uint16_t addr) _SC(54);
void _SYS_vbuf_fill(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word, uint16_t count) _SC(20);
void _SYS_vbuf_seqi(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t index, uint16_t count) _SC(124);
void _SYS_vbuf_write(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t count) _SC(125);
void _SYS_vbuf_writei(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count) _SC(47);
void _SYS_vbuf_wrect(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count, uint16_t lines, uint16_t src_stride, uint16_t addr_stride) _SC(21);
void _SYS_vbuf_spr_resize(struct _SYSVideoBuffer *vbuf, unsigned id, unsigned width, unsigned height) _SC(19);
void _SYS_vbuf_spr_move(struct _SYSVideoBuffer *vbuf, unsigned id, int x, int y) _SC(35);

uint32_t _SYS_audio_play(const struct _SYSAudioModule *mod, _SYSAudioHandle *h, enum _SYSAudioLoopType loop) _SC(50);
uint32_t _SYS_audio_isPlaying(_SYSAudioHandle h) _SC(127);
void _SYS_audio_stop(_SYSAudioHandle h) _SC(52);
void _SYS_audio_pause(_SYSAudioHandle h) _SC(128);
void _SYS_audio_resume(_SYSAudioHandle h) _SC(129);
int32_t _SYS_audio_volume(_SYSAudioHandle h) _SC(130);
void _SYS_audio_setVolume(_SYSAudioHandle h, int32_t volume) _SC(131);
uint32_t _SYS_audio_pos(_SYSAudioHandle h) _SC(132);

uint32_t _SYS_asset_slotTilesFree(_SYSAssetSlot slot) _SC(63);
void _SYS_asset_slotErase(_SYSAssetSlot slot) _SC(133);
uint32_t _SYS_asset_loadStart(struct _SYSAssetLoader *loader, struct _SYSAssetGroup *group, _SYSAssetSlot slot, _SYSCubeIDVector cv) _SC(134);
void _SYS_asset_loadFinish(struct _SYSAssetLoader *loader) _SC(135);
uint32_t _SYS_asset_findInCache(struct _SYSAssetGroup *group, _SYSCubeIDVector cv) _SC(136);

void _SYS_image_memDraw(uint16_t *dest, const struct _SYSAssetImage *im, unsigned dest_stride, unsigned frame) _SC(137);
void _SYS_image_memDrawRect(uint16_t *dest, const struct _SYSAssetImage *im, unsigned dest_stride, unsigned frame, struct _SYSInt2 *srcXY, struct _SYSInt2 *size) _SC(138);
void _SYS_image_BG0Draw(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, uint16_t addr, unsigned frame) _SC(139);
void _SYS_image_BG0DrawRect(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, uint16_t addr, unsigned frame, struct _SYSInt2 *srcXY, struct _SYSInt2 *size) _SC(140);
void _SYS_image_BG1Draw(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, struct _SYSInt2 *destXY, unsigned frame) _SC(141);
void _SYS_image_BG1DrawRect(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, struct _SYSInt2 *destXY, unsigned frame, struct _SYSInt2 *srcXY, struct _SYSInt2 *size) _SC(142);
void _SYS_image_BG2Draw(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, uint16_t addr, unsigned frame) _SC(143);
void _SYS_image_BG2DrawRect(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, uint16_t addr, unsigned frame, struct _SYSInt2 *srcXY, struct _SYSInt2 *size) _SC(144);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
