/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
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
 *
 * Normally you would disable debugging by linking your binary as a release
 * build. However, sometimes it can be useful to have a build with symbols
 * but without ASSERTs and LOGs. For example, this can be used to isolate
 * problems that, for whatever reason, only show up with ASSERTs or LOGs
 * disabled. You can do this by setting the -DNO_ASSERT or -DNO_LOG
 * compiler options.
 */

#define DEBUG_ONLY(_x)  do { if (_SYS_lti_isDebug()) { _x } } while (0)

#ifdef NO_LOG
#   define LOG(...)
#else
#   define LOG(...)     do { if (_SYS_lti_isDebug()) _SYS_lti_log(__VA_ARGS__); } while (0)
#endif

#ifdef NO_ASSERT
#   define ASSERT(_x)
#else
#   define ASSERT(_x) do { \
        if (_SYS_lti_isDebug() && !(_x)) { \
            _SYS_lti_log("ASSERT failure at %s:%d, (%s)\n", __FILE__, __LINE__, #_x); \
            _SYS_abort(); \
        } \
    } while (0)
#endif

/// Convenient trace macros for printing the values of variables
#define LOG_INT(_x)     LOG("%s = %d\n", #_x, (_x));
#define LOG_HEX(_x)     LOG("%s = 0x%08x\n", #_x, (_x));
#define LOG_FLOAT(_x)   LOG("%s = %f\n", #_x, (_x));
#define LOG_STR(_x)     LOG("%s = \"%s\"\n", #_x, (const char*)(_x));
#define LOG_INT2(_x)    LOG("%s = (%d, %d)\n", #_x, (_x).x, (_x).y);
#define LOG_FLOAT2(_x)  LOG("%s = (%f, %f)\n", #_x, (_x).x, (_x).y);

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

