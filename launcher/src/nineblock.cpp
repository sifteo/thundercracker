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

#include "nineblock.h"
#include "assets.gen.h"
using namespace Sifteo;


unsigned NineBlock::frame(unsigned pattern, unsigned angle, bool corner)
{
    ASSERT(pattern < 16);
    ASSERT(corner < 2);
    return pattern*8 + corner*4 + (angle % 4);
}


void NineBlock::generate(unsigned identity, MainMenuItem::IconBuffer &icon)
{
    Random prng(identity);

    // The unique elements: Random pattern and rotation for the edge piece,
    // but only a random pattern for the corner (since the rounded bit has
    // to line up)

    unsigned edgePattern = prng.randrange(16);
    unsigned edgeAngle = prng.randrange(4);
    unsigned cornerPattern = prng.randrange(16);
    unsigned cornerAngle = 0;

    // Plot the images!

    icon.image(vec(4,4), NineBlock_Center);

    icon.image(vec(0,4), NineBlock_Shapes, frame(edgePattern, edgeAngle++, false));
    icon.image(vec(4,0), NineBlock_Shapes, frame(edgePattern, edgeAngle++, false));
    icon.image(vec(8,4), NineBlock_Shapes, frame(edgePattern, edgeAngle++, false));
    icon.image(vec(4,8), NineBlock_Shapes, frame(edgePattern, edgeAngle++, false));

    icon.image(vec(0,0), NineBlock_Shapes, frame(cornerPattern, cornerAngle++, true));
    icon.image(vec(8,0), NineBlock_Shapes, frame(cornerPattern, cornerAngle++, true));
    icon.image(vec(8,8), NineBlock_Shapes, frame(cornerPattern, cornerAngle++, true));
    icon.image(vec(0,8), NineBlock_Shapes, frame(cornerPattern, cornerAngle++, true));
};
