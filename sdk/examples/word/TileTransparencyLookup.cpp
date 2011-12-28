#include <sifteo.h>
#include "TileTransparencyLookup.h"

extern const uint8_t* tileTransparencyLookupData[];

TransparencyType getTransparencyType(ImageIndex imageIndex,
                                     unsigned frame,
                                     unsigned tileX,
                                     unsigned tileY)
{
    ASSERT(imageIndex >= 0);
    ASSERT(imageIndex < NumImageIndexes);
    ASSERT(tileX < 16);
    ASSERT(tileY < 16);

    const uint8_t* lookupData = tileTransparencyLookupData[imageIndex];
    const unsigned BITS_PER_TILE = 2;
    const unsigned TILE_BITS_MASK = 0x3;
    const unsigned TILES_PER_BYTE = 8 / BITS_PER_TILE;
    const unsigned TILES_PER_ROW = 16;
    const unsigned TILES_PER_FRAME = TILES_PER_ROW * TILES_PER_ROW;

    unsigned tileIndex = frame * TILES_PER_FRAME + tileY * TILES_PER_ROW + tileX;
    unsigned byteIndex = tileIndex / (TILES_PER_BYTE);
    uint8_t byte = lookupData[byteIndex];
    uint8_t shift = (tileIndex % TILES_PER_BYTE) * BITS_PER_TILE;
    uint8_t result = ((byte >> shift) & TILE_BITS_MASK);
    ASSERT (result < NumTransparencyTypes);
    return (TransparencyType)result;
}
