/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_MACROS_H
#define _SIFTEO_MACROS_H

#ifdef SIFTEO_SIMULATOR
#include <stdio.h>
#include <assert.h>
#endif

/*
 * ASSERT is not an error handling mechanism! They are only present in
 * simulator builds. Use ASSERTs to catch errors that shouldn't be
 * possible; cases where without the ASSERT, you'd be crashing or
 * corrupting memory. Use ASSERT to catch errors, not to fix them!
 */

#ifdef SIFTEO_SIMULATOR
#   ifdef DEBUG
#      define DEBUG_LOG(_x)   printf _x
#   else
#      define DEBUG_LOG(_x)
#   endif
#   define LOG(_x)            printf _x
#   define ASSERT(_x)         assert(_x)
#   define DEBUG_ONLY(x)      x
#else
#   define DEBUG_LOG(_x)
#   define LOG(_x)
#   define ASSERT(_x)
#   define DEBUG_ONLY(x)
#endif

// Produces a 'size of array is negative' compile error when the assert fails
#define STATIC_ASSERT(_x)  ((void)sizeof(char[1 - 2*!(_x)]))

#ifndef MIN
#define MIN(a,b)   ((a) < (b) ? (a) : (b))
#define MAX(a,b)   ((a) > (b) ? (a) : (b))
#endif

#ifndef arraysize
#define arraysize(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef offsetof
#define offsetof(t,m)  ((uintptr_t)(uint8_t*)&(((t*)0)->m))
#endif

#endif
