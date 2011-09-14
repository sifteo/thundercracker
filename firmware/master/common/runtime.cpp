/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "runtime.h"

jmp_buf Runtime::jmpExit;


void Runtime::run()
{
    if (setjmp(jmpExit))
	return;

    siftmain();
}

void Runtime::exit()
{
    longjmp(jmpExit, 1);
}
