/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
using namespace Sifteo;

volatile int x, y;

struct Fizzbin {
    Fizzbin() {
        x = 10;
        y = 18;
    }
    
    ~Fizzbin() {
        x = 11;
    }
};

Fizzbin fluff;

void siftmain()
{
    x = 5;
}