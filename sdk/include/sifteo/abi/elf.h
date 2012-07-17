/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_ABI_ELF_H
#define _SIFTEO_ABI_ELF_H

#include <sifteo/abi/types.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
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

// SVM-specific program header types
#define _SYS_ELF_PT_METADATA        0x7000f001      // Metadata key/value dictionary
#define _SYS_ELF_PT_LOAD_FASTLZ     0x7000f002      // PT_LOAD, with FastLZ (Level 1) compression

struct _SYSMetadataKey {
    uint16_t    stride;     // Byte offset from this value to the next
    uint16_t    key;        // _SYS_METADATA_*
};

// Maximum size for a single metadata value
#define _SYS_MAX_METADATA_ITEM_BYTES    0x100

// System Metadata keys
#define _SYS_METADATA_NONE          0x0000  // Ignored. (padding)
#define _SYS_METADATA_UUID          0x0001  // Binary UUID for this specific build
#define _SYS_METADATA_BOOT_ASSET    0x0002  // Array of _SYSMetadataBootAsset
#define _SYS_METADATA_TITLE_STR     0x0003  // Human readable game title string
#define _SYS_METADATA_PACKAGE_STR   0x0004  // DNS-style package string
#define _SYS_METADATA_VERSION_STR   0x0005  // Version string
#define _SYS_METADATA_ICON_96x96    0x0006  // _SYSMetadataImage
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

/*
 * Entry point. Our standard entry point is main(), with no arguments
 * or return values, declared using C linkage.
 */

#ifndef NOT_USERSPACE
void main(void);
#endif

/*
 * Link-time intrinsics.
 *
 * These functions are replaced during link-time optimization.
 *
 * Logging supports many standard printf() format specifiers,
 * as documented in sifteo/macros.h
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


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
