/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
using namespace Sifteo;

#ifndef NUM_CUBES
#  define NUM_CUBES 3
#endif

//static Cube cubes[NUM_CUBES];

void main()
{
    static uint16_t a[5];
    _SYS_memcpy16(&a[2], &a[3], 1);
}