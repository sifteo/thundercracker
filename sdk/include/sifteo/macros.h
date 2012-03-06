/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_MACROS_H
#define _SIFTEO_MACROS_H

#include <sifteo/abi.h>

#define STRINGIFY(_x)   #_x
#define TOSTRING(_x)    STRINGIFY(_x)
#define SOURCELOC_STR   __FILE__ ":" TOSTRING(__LINE__)

#ifdef FW_BUILD

    /*
     * Firmware internal debug macros
     *
     * Variants of the standard LOG and ASSERT macros, for building game code
     * which is to be compiled into a firmware image. Logging and ASSERTs are
     * always disabled on hardware builds and enabled on simulation builds.
     */

    #include <stdio.h>
    #include <assert.h>

    #ifdef _NEWLIB_STDIO_H
    #define printf      iprintf
    #define sprintf     siprintf
    #define snprintf    sniprintf
    #define vsnprintf   vsniprintf
    #endif

    #ifdef SIFTEO_SIMULATOR
    #   ifdef DEBUG
    #      define DEBUG_LOG(_x)   printf _x
    #   else
    #      define DEBUG_LOG(_x)
    #   endif
    #   define LOG(_x)            printf _x
    #   define ASSERT(_x)         assert(_x)
    #   define DEBUG_ONLY(x)      x
    #   define UART(_x)
    #else
    #   define DEBUG_LOG(_x)
    #   define LOG(_x)
    #   define ASSERT(_x)
    #   define DEBUG_ONLY(x)
    #   define UART(_x)           Usart::Dbg.write(_x)
    #endif

#else  // FW_BUILD

    /*
     * Virtualized debug macros
     *
     * These LOG and ASSERT macros are eliminated at link-time on release builds.
     * On debug builds, the actual log messages and assert failure messages are
     * included in a separate debug symbol ELF section, not in your application's
     * normal data section.
     */

    #define LOG(_x) do { \
        if (_SYS_lti_isDebug()) \
            _SYS_lti_log _x ; \
    } while (0)

    #define DEBUG_ONLY(_x) do { \
        if (_SYS_lti_isDebug()) { \
            _x \
        } \
    } while (0)

    #define ASSERT(_x) do { \
        if (_SYS_lti_isDebug() && !(_x)) { \
            _SYS_lti_log("ASSERT failure at " SOURCELOC_STR ", (" #_x ")"); \
            _SYS_abort(); \
        } \
    } while (0)

#endif  // FW_BUILD

// Produces a 'size of array is negative' compile error when the assert fails
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
