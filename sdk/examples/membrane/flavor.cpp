/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "game.h"


void Flavor::randomize()
{
    // Always choose two different elements    
    elements[0] = Game::random.randint(FIRST_ELEMENT, LAST_ELEMENT);
    elements[1] = Game::random.randrange(FIRST_ELEMENT, LAST_ELEMENT);
    if (elements[1] >= elements[0])
        elements[1]++;
        
    // Sort elements in ascending order, so that each pair is unique
    if (elements[0] < elements[1]) {
        uint8_t t = elements[0];
        elements[0] = elements[1];
        elements[1] = t;
    }
}
    
const PinnedAssetImage &Flavor::getAsset()
{
    return Particle0;
}

unsigned Flavor::getFrameBase()
{
    static const uint8_t table[] = {
        /*           Earth  Water  Fire   Air  */
        /* Earth */  0*4,   6*4,   5*4,   4*4,
        /* Water */  6*4,   1*4,   9*4,   8*4,
        /* Fire  */  5*4,   9*4,   2*4,   7*4,
        /* Air   */  4*4,   8*4,   7*4,   3*4,
    };
    
    return table[elements[0] | elements[1]*4];
}

const PinnedAssetImage &Flavor::getWarpAsset()
{
    return Particle0Warp;
}

unsigned Flavor::getWarpFrameBase()
{
    return elements[0] == elements[1] ? 0 : 6;
}

bool Flavor::hasElement(unsigned e) const
{
    for (unsigned i = 0; i < arraysize(elements); i++)
        if (elements[i] == e)
            return true;
    return false;
}

void Flavor::replaceElement(unsigned from, unsigned to)
{
    for (unsigned i = 0; i < arraysize(elements); i++)
        if (elements[i] == from)
            elements[i] = to;
}

unsigned Flavor::getMatchBit() const
{
    if (elements[0] == elements[1])
        return 1 << elements[0];
    else
        return 0;
}

