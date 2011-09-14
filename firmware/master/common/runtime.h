/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_RUNTIME_H
#define _SIFTEO_RUNTIME_H

#include <setjmp.h>
#include <sifteo/abi.h>


/**
 * Game code runtime environment
 */

class Runtime {
 public:
    static void run();
    static void exit();

    static bool checkUserPointer(void *ptr, intptr_t size, bool allowNULL=false) {
	/*
	 * XXX: Validate a memory address that was provided to us by the game,
	 *      make sure it isn't outside the game's sandbox region. This code
	 *      MUST use overflow-safe arithmetic!
	 *
	 * Also checks for NULL pointers, assuming allowNULL isn't true.
	 */

	if (!ptr && !allowNULL)
	    return false;

	return true;
    }
 
 private:
    static jmp_buf jmpExit;
};


#endif
