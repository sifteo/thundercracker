/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_MACROS_H
#define _SIFTEO_MACROS_H

#ifdef SIFTEO_SIMULATOR
#include <stdio.h>
#include <assert.h>
#else
#ifdef FW_BUILD // ie, not a game build
#include "usart.h"
#endif
#endif

/*
 * When we're doing an embedded build with Newlib, use integer-only
 * printf() functions, so we can avoid linking in atod() and all of the
 * giant dependencies that this entails.
 *
 * This means that specifiers like %f and %g will not work!
 *
 * (Of course, they wouldn't work anyway if we were running without dynamic
 * memory allocation support, as atod requires a malloc'ed buffer.)
 */
 
#ifdef _NEWLIB_STDIO_H
#define printf      iprintf
#define sprintf     siprintf
#define snprintf    sniprintf
#define vsnprintf   vsniprintf
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
#if QTCREATOR
#   define ASSERT(_x)         if(!(_x)) {asm("int $0x3");} 
#elif QTCREATOR_TERMINAL // FIXME figure out how to trap int 3 in Qt Terminal
extern void assertWrapper(bool b);
#   define ASSERT(_x)        assertWrapper(_x)
#else
#   define ASSERT(_x)         assert(_x)
#endif
#   define DEBUG_ONLY(x)      x
#else
#   define DEBUG_LOG(_x)
#   define LOG(_x)
#   define ASSERT(_x)
#   define DEBUG_ONLY(x)
#endif

// debug dump to uart - only defined for firmware internal code, not games
#ifdef SIFTEO_SIMULATOR
#   define UART(_x)
#else
#   define UART(_x)     Usart::Dbg.write(_x)
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

#endif
