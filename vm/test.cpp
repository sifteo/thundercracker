/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
using namespace Sifteo;

Cube cube(5);

volatile int x;

void __attribute__ ((noinline)) arbl(volatile int*base)
{
    base[500] = 1;
}

void siftmain()
{
    arbl(&x);
    arbl(1 + &x);
}