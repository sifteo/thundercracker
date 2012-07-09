/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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


void NineBlock::generate(unsigned identity, Sifteo::RelocatableTileBuffer<12,12> &icon)
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
