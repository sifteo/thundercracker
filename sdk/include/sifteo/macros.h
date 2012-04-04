/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_MACROS_H
#define _SIFTEO_MACROS_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>

#define NOINLINE        __attribute__ ((noinline))

#define STRINGIFY(_x)   #_x
#define TOSTRING(_x)    STRINGIFY(_x)
#define SRCLINE         __FILE__ ":" TOSTRING(__LINE__)

/**
 * These LOG and ASSERT macros are eliminated at link-time on release builds.
 * On debug builds, the actual log messages and assert failure messages are
 * included in a separate debug symbol ELF section, not in your application's
 * normal data section.
 */

#define LOG(_x) do { \
    if (_SYS_lti_isDebug()) \
        _SYS_lti_log _x ; \
} while (0)

#define DEBUG_LOG(_x)   LOG(_x)

#define DEBUG_ONLY(_x) do { \
    if (_SYS_lti_isDebug()) { \
        _x \
    } \
} while (0)

#define ASSERT(_x) do { \
    if (_SYS_lti_isDebug() && !(_x)) { \
        _SYS_lti_log("ASSERT failure at %s:%d, (%s)\n", __FILE__, __LINE__, #_x); \
        _SYS_abort(); \
    } \
} while (0)

/// Convenient trace macros for printing the values of variables
#define LOG_INT(_x)     LOG((#_x " = %d\n", (_x)));
#define LOG_HEX(_x)     LOG((#_x " = 0x%08x\n", (_x)));
#define LOG_FLOAT(_x)   LOG((#_x " = %f\n", (_x)));
#define LOG_STR(_x)     LOG((#_x " = \"%s\"\n", &(_x)));
#define LOG_INT2(_x)    LOG((#_x " = (%d, %d)\n", (_x).x, (_x).y));
#define LOG_FLOAT2(_x)  LOG((#_x " = (%f, %f)\n", (_x).x, (_x).y));

/// Produces a 'size of array is negative' compile error when the assert fails
#define STATIC_ASSERT(_x)  ((void)sizeof(char[1 - 2*!(_x)]))

#ifndef MIN
#define MIN(a,b)   ((a) < (b) ? (a) : (b))
#define MAX(a,b)   ((a) > (b) ? (a) : (b))
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef arraysize
#define arraysize(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef offsetof
#define offsetof(t,m)  ((uintptr_t)(uint8_t*)&(((t*)0)->m))
#endif

#endif
