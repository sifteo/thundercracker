/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Low level hardware setup for the STM32 board.
 */

#include <sifteo/abi.h>
#include "radio.h"
#include "runtime.h"

extern "C" void *_stack;

extern "C" void _start()
{
    Radio::
}

__attribute__ ((section (".vectors"))) uintptr_t vector_table[] = {
    (uintptr_t) &_stack,
    (uintptr_t) &_start,
};

