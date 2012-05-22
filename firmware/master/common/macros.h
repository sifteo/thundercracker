/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef MACROS_H
#define MACROS_H

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>

#define APP_TITLE       "Sifteo Cubes"
#define APP_COPYRIGHT   "Copyright <c> 2011-2012 Sifteo, Inc. All rights reserved."

#define STRINGIFY(_x)   #_x
#define TOSTRING(_x)    STRINGIFY(_x)
#define SRCLINE         __FILE__ ":" TOSTRING(__LINE__)

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

#define ALWAYS_INLINE   __attribute__ ((always_inline))
#define NEVER_INLINE    __attribute__ ((noinline))

#define LIKELY(x)       __builtin_expect((x), 1)
#define UNLIKELY(x)     __builtin_expect((x), 0)

#ifndef MIN
#define MIN(a,b)        ((a)<(b)?(a):(b))
#define MAX(a,b)        ((a)>(b)?(a):(b))
#endif

#ifndef FASTCALL
#   ifdef __i386__
#       define FASTCALL __attribute__ ((fastcall))
#   else
#       define FASTCALL
#   endif
#endif

#ifndef PRIu64
#   if defined(_WIN32)
#      define PRIu64 "I64u"
#   else
#      error No definition for PRIu64. \
             This is supposed to come from inttypes.h. \
             Is __STDC_FORMAT_MACROS defined?
#   endif
#endif

template <typename T> inline T clamp(T value, T low, T high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

/*
 * Scale 'value' between the ranges specified by min1 - max1 and min2 - max2
 */
template <typename T> inline T scale(T value, T min1, T max1, T min2, T max2)
{
    ASSERT(min1 < max1);
    ASSERT(min2 < max2);

    T range1 = (max1 - min1);
    T range2 = (max2 - min2);
    return (((value - min1) * range2) / range1) + min2;
}

/**
 * Round up to the nearest 'a'-byte boundary, assuming 'a' is a
 * constant power of two.
 */
template <unsigned a> inline unsigned roundup(unsigned value)
{
    STATIC_ASSERT((a & (a - 1)) == 0);
    return (value + (a - 1)) & ~(a - 1);
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
