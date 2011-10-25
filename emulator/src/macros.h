/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _MACROS_H
#define _MACROS_H

#define APP_TITLE   "Thundercracker"

/*
 * Tick functions should always be inlined, otherwise the function
 * call cost ends up being many times the cost of actually checking
 * the timer condition.
 */

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

#endif
