/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _MACROS_H
#define _MACROS_H

#include <stdint.h>

#define APP_TITLE      "Thundercracker"
#define APP_COPYRIGHT  "Copyright <c> 2011-2012 Sifteo, Inc. All rights reserved."

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

#ifndef PRIu64
#   if defined(_WIN32)
#      define PRIu64 "I64u"
#   elif defined(_LP64)
#      define PRIu64 "lu"
#   else
#      define PRIu64 "llu"
#   endif
#endif

#endif
