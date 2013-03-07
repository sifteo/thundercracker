/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_ABI_TYPES_H
#define _SIFTEO_ABI_TYPES_H

#ifdef NOT_USERSPACE
#   include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Standard integer types. This is a subset of what's in stdint.h,
 * but we define them ourselves since system headers are not available.
 *
 * If we're doing a non-userspace build, however, these are pulled in
 * from stdint.h above.
 */

#ifndef NOT_USERSPACE
    typedef signed char int8_t;
    typedef unsigned char uint8_t;
    typedef signed short int16_t;
    typedef unsigned short uint16_t;
    typedef signed int int32_t;
    typedef unsigned int uint32_t;
    typedef signed long long int64_t;
    typedef unsigned long long uint64_t;
    typedef signed long intptr_t;
    typedef unsigned long uintptr_t;
#endif

// We can use 'bool' even in pure C code.
#ifndef __cplusplus
    typedef uint8_t bool;
#endif

/*
 * Basic data types which are valid across the user/system boundary.
 */

#define _SYS_NUM_CUBE_SLOTS     24      // Total supported cube slots
#define _SYS_CUBE_ID_INVALID    0xFF    // Reserved _SYSCubeID value
#define _SYS_BATTERY_MAX        0x10000 // Battery levels are 16.16 fixed point

typedef uint8_t _SYSCubeID;             // Cube slot index
typedef uint8_t _SYSNeighborID;         // Neighbored object ID (superset of _SYSCubeID)
typedef int8_t _SYSSideID;              // Cube side index
typedef uint32_t _SYSCubeIDVector;      // One bit for each cube slot, MSB-first
typedef uint8_t _SYSAssetSlot;          // Ordinal for one of the game's asset slots

/*
 * Small vector types
 */

struct _SYSInt2 {
    int32_t x, y;
};

struct _SYSInt3 {
    int32_t x, y, z;
};

union _SYSByte4 {
    struct {
        int8_t x, y, z, w;
    };
    uint32_t value;
};

/*
 * Neighbors
 */

#define _SYS_NEIGHBOR_TYPE_MASK     0xE0    // Mask for neighbor type bits
#define _SYS_NEIGHBOR_ID_MASK       0x1F    // Mask for neighbor ID bits

#define _SYS_NEIGHBOR_CUBE          0x00    // Neighbored to normal cube
#define _SYS_NEIGHBOR_BASE          0x20    // Neighbored to base

#define _SYS_NEIGHBOR_NONE          0xFF    // No neighbor

union _SYSNeighborState {
    uint32_t value;
    _SYSNeighborID sides[4];
};

/*
 * Motion reporting
 */

#define _SYS_MOTION_MAX_ENTRIES     256     // Max size of motion buffer
#define _SYS_MOTION_TIMESTAMP_NS    250000  // Nanoseconds per timestamp unit (0.25 ms)
#define _SYS_MOTION_TIMESTAMP_HZ    4000    // Reciprocal of _SYS_MOTION_TIMESTAMP_NS

struct _SYSMotionBufferHeader {
    uint8_t tail;           /// Index of the next empty slot to write into
    uint8_t last;           /// Index of last buffer slot. If tail > size, tail wraps to 0
    uint8_t rate;           /// Requested capture rate, in timestamp units per sample
    uint8_t reserved;       /// Initialize to zero
    /*
     * Followed by variable-size array of _SYSByte4.
     *
     * The 'w' field is used for a timestamp, encoded as a (delta - 1) since
     * the last mesurement, in units of _SYS_MOTION_TIMESTAMP_NS. This means
     * the maximum representable delta is exactly 64 milliseconds. Deltas of
     * longer than this will be represented by duplicating samples in the buffer.
     */
};

struct _SYSMotionBuffer {
    struct _SYSMotionBufferHeader header;
    union _SYSByte4 samples[_SYS_MOTION_MAX_ENTRIES];
};

struct _SYSMotionMedianAxis {
    int8_t median;
    int8_t minimum;
    int8_t maximum;
};

struct _SYSMotionMedian {
    struct _SYSMotionMedianAxis axes[3];
};

/*
 * Type bits, for use in the 'tag' for the low-level _SYS_log() handler.
 * Normally these don't need to be used in usermode code, they're inserted
 * automatically by slinky when expanding _SYS_lti_log().
 */

#define _SYS_LOGTYPE_FMT            0       // param = strtab offest
#define _SYS_LOGTYPE_STRING         1       // param = 0, v1 = ptr
#define _SYS_LOGTYPE_HEXDUMP        2       // param = length, v1 = ptr
#define _SYS_LOGTYPE_SCRIPT         3       // param = script type

#define _SYS_SCRIPT_NONE            0       // Normal logging
#define _SYS_SCRIPT_LUA             1       // Built-in Lua interpreter

/*
 * Internal state of the Pseudorandom Number Generator, maintained in user RAM.
 */

struct _SYSPseudoRandomState {
    uint32_t a, b, c, d;
};

/*
 * Versioning.
 *
 * SYS_FEATUREs is a mask of features supported by the current version of the OS.
 *
 * NB: be sure to add any new features to _SYS_FEATURE_ALL.
 */

#define _SYS_OS_VERSION_MASK        0xffffff
#define _SYS_HW_VERSION_SHIFT       24

// defaults for earlier OS versions that don't support _SYS_version()
#define _SYS_OS_VERSION_NONE        0x00
#define _SYS_HW_VERSION_NONE        0x00

#define _SYS_FEATURE_SYS_VERSION    (1 << 0)
#define _SYS_FEATURE_ALL            (_SYS_FEATURE_SYS_VERSION)

/*
 * Hardware IDs are 64-bit numbers that uniquely identify a
 * particular cube. A valid HWIDs never contains 0xFF bytes.
 */

#define _SYS_HWID_BYTES         8
#define _SYS_HWID_BITS          64
#define _SYS_INVALID_HWID       ((uint64_t)-1)

/*
 * Filesystem
 */

#define _SYS_FS_VOL_GAME            0x4d47
#define _SYS_FS_VOL_LAUNCHER        0x4e4c

#define _SYS_FS_MAX_OBJECT_KEYS     256
#define _SYS_FS_MAX_OBJECT_SIZE     4080

// Opaque nonzero ID for a filesystem volume
typedef uint32_t _SYSVolumeHandle;      

// Application-defined ID for a key in our key/value object store
typedef uint8_t _SYSObjectKey;

struct _SYSFilesystemInfo {
    uint32_t unitSize;          // Size of allocation unit, in bytes
    uint32_t totalUnits;        // Total number of allocation units on device
    uint32_t freeUnits;         // Number of free or immediately-recyclable units
    uint32_t systemUnits;       // Used by system data (cube pairing data, asset cache)
    uint32_t launcherElfUnits;  // Used by launcher ELF volumes
    uint32_t launcherObjUnits;  // Used by StoredObjects owned by launcher
    uint32_t gameElfUnits;      // Used by game ELF volumes
    uint32_t gameObjUnits;      // Used by StoredObjects owned by games
    uint32_t selfElfUnits;      // Used by the currently executing volume (may overlap with above)
    uint32_t selfObjUnits;      // StoredObjects owned by the currently executing volume (may overlap with above)
};

/*
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

/*
 * System error codes, returned by some syscalls:
 *   - _SYS_fs_objectRead
 *   - _SYS_fs_objectWrite
 *
 * Where possible, these numbers line up with standard POSIX errno values.
 */

#define _SYS_ENOENT     -2      // No such object found
#define _SYS_EFAULT     -14     // Bad address
#define _SYS_EINVAL     -22     // Invalid argument
#define _SYS_ENOSPC     -28     // No space left on device

/*
 * Flags for _SYS_shutdown()
 */

#define _SYS_SHUTDOWN_WITH_UI       (1 << 0)    // Present shutdown user interface, allow cancellation


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
