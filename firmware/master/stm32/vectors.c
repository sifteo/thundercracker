/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Vector table. Defined in C rather than C++, so that we
 * can use C99 struct initializers.
 */

#include "vectors.h"

extern unsigned _stack;

__attribute__ ((section (".vectors"))) struct IVT_t vector_table =
{
    .stack = &_stack,
    .Reset = _start,
    .EXTI0 = _start,
};

