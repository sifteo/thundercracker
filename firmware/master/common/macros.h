/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef MACROS_H
#define MACROS_H

#define STRINGIFY(_x)   #_x
#define TOSTRING(_x)    STRINGIFY(_x)
#define SRCLINE         __FILE__ ":" TOSTRING(__LINE__)

/*
 * Firmware internal debug macros
 *
 * Variants of the standard LOG and ASSERT macros, for building game code
 * which is to be compiled into a firmware image. Logging and ASSERTs are
 * always disabled on hardware builds and enabled on simulation builds.
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>

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
#   define SECTION(_x)
#else
#   define DEBUG_LOG(_x)
#   define LOG(_x)
#   define ASSERT(_x)
#   define DEBUG_ONLY(x)
#   include                   "usart.h"
#   define UART(_x)           Usart::Dbg.write(_x)
#   define SECTION(_x)        __attribute__((section(_x)))
#endif

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

template <typename T> inline T clamp(const T& value, const T& low, const T& high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

template <typename T> inline T abs(const T& value)
{
    if (value < 0) {
        return -value;
    }
    return value;
}

/**
 * Compute the unsigned remainder from dividing two signed integers.
 */
unsigned inline umod(int a, int b)
{
    int r = a % b;
    if (r < 0)
        r += b;
    return r;
}

/**
 * Saturating 16x16=32 multiply.
 *
 * If the operands are greater than 0xFFFF, the result will automatically
 * saturate to 0xFFFFFFFF. Guaranteed not to overflow a uint32_t.
 */
uint32_t inline mulsat16x16(uint32_t a, uint32_t b)
{
    if (a > 0xFFFF) return 0xFFFFFFFF;
    if (b > 0xFFFF) return 0xFFFFFFFF;
    return a * b;
}


#endif
