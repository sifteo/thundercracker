/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker launcher
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Shared memory resources which are multiplexed between the main menu,
 * loading animations, and any built-in applets.
 */

#pragma once
#include <sifteo.h>
#include "defaultcuberesponder.h"

namespace Shared {

    extern Sifteo::VideoBuffer video[CUBE_ALLOCATION];
	extern Sifteo::TiltShakeRecognizer motion[CUBE_ALLOCATION];
	extern Sifteo::SystemTime connectTime[CUBE_ALLOCATION];
    extern DefaultCubeResponder cubeResponder[CUBE_ALLOCATION];

    extern Sifteo::Random random;

    static const unsigned NUM_SLOTS = 2;
    extern Sifteo::AssetSlot primarySlot;   // General purpose
    extern Sifteo::AssetSlot iconSlot;      // Used only for main menu icons

    static const unsigned MAX_ITEMS = 32;

    // Enough room for all menu items, and for our own single AssetGroup
    typedef Sifteo::AssetConfiguration<MAX_ITEMS + 1> AssetConfiguration;

}
