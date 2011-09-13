/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>

int main()
{
    Sifteo::System::init();

    while (1) {
	Sifteo::System::draw();
    }

    return 0;
}
