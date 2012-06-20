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

#define APP_TITLE               "Sifteo Cubes"
#define APP_COPYRIGHT_ASCII     "Copyright <c> 2011-2012 Sifteo, Inc. All rights reserved."
#define APP_COPYRIGHT_LATIN1    "Copyright \xa9 2011-2012 Sifteo, Inc. All rights reserved."

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

#ifndef UINT16_MAX
#define UINT16_MAX 0xffff
#endif

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

#ifndef arraysize
#define arraysize(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef offsetof
#define offsetof(t,m)  ((uintptr_t)(uint8_t*)&(((t*)0)->m))
#endif

#define ALWAYS_INLINE   inline __attribute__ ((always_inline))
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

template <typename T> ALWAYS_INLINE T clamp(T value, T low, T high)
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
 * Scale 'value' between the ranges specified by minIn/maxIn and minOut/maxOut.
 *
 * 'value' is expected to be in the range minIn - maxIn.
 */
template <typename T> ALWAYS_INLINE T scale(T value, T minIn, T maxIn, T minOut, T maxOut)
{
    ASSERT(minIn < maxIn);
    ASSERT(minOut < maxOut);

    T rangeIn = (maxIn - minIn);
    T rangeOut = (maxOut - minOut);
    return (((value - minIn) * rangeOut) / rangeIn) + minOut;
}

/**
 * Round up to the nearest 'a'-byte boundary, assuming 'a' is a
 * constant power of two.
 */
template <unsigned a> ALWAYS_INLINE unsigned roundup(unsigned value)
{
    STATIC_ASSERT((a & (a - 1)) == 0);
    return (value + (a - 1)) & ~(a - 1);
}

/**
 * Ceiling division. Divide, rounding up instead of down.
 */
template <typename T> ALWAYS_INLINE T ceildiv(T numerator, T denominator) {
    return (numerator + (denominator - 1)) / denominator;
}

/**
 * Compute the unsigned remainder from dividing two signed integers.
 */
unsigned ALWAYS_INLINE umod(int a, int b)
{
    int r = a % b;
    if (r < 0)
        r += b;
    return r;
}

/**
 * Swap two values of any assignable type.
 */
template <typename T> ALWAYS_INLINE void swap(T &a, T &b)
{
    T temp = a;
    a = b;
    b = temp;
}

/**
 * Saturating 16x16=32 multiply.
 *
 * If the operands are greater than 0xFFFF, the result will automatically
 * saturate to 0xFFFFFFFF. Guaranteed not to overflow a uint32_t.
 */
uint32_t ALWAYS_INLINE mulsat16x16(uint32_t a, uint32_t b)
{
    if (a > 0xFFFF) return 0xFFFFFFFF;
    if (b > 0xFFFF) return 0xFFFFFFFF;
    return a * b;
}

/**
 * Check the alignment of a pointer
 */
template <typename T> ALWAYS_INLINE bool isAligned(const T *ptr, unsigned alignment=4)
{
    STATIC_ASSERT((alignment & (alignment - 1)) == 0);
    return (reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0;
}


#endif
