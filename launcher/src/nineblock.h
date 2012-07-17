/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>


/**
 * The NineBlock pattern is based on a classic quilting pattern, using
 * a concept borrowed from Identicon: we can generate a visually distinctive
 * icon that corresponds with a hash.
 *
 * Our NineBlock pattern has a fixed center square (for displaying a mini-icon)
 * and a fixed color (since the assets must be pre-baked). Still, this gives us
 * nearly 1024 different icons.
 */

class NineBlock
{
public:

    /**
     * Generate a 12x12 tile image based on the 'identity' value.
     * This value is used as the seed for a PRNG that generates the
     * characteristics of this icon.
     */
    static void generate(unsigned identity, Sifteo::RelocatableTileBuffer<12,12> &icon);

private:

    /// Generate an asset frame number
    static unsigned frame(unsigned pattern, unsigned angle, bool corner);
};
