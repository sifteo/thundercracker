/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "imagedecoder.h"
#include "cube.h"
#include "cubeslots.h"


bool ImageDecoder::init(const _SYSAssetImage *userPtr, _SYSCubeID cid)
{
    /*
     * Validate user pointers, and load the header and base address for
     * this AssetImage on this cube.
     *
     * Returns 'true' on success, 'false' on failure due to bad
     * userspace-provided data.
     */

    if (!SvmMemory::copyROData(header, userPtr))
        return false;

    if (cid >= _SYS_NUM_CUBE_SLOTS)
        return false;
    CubeSlot &cube = CubeSlots::instances[cid];

    // Map and validate per-cube data.
    // We don't need to map the _SYSAssetGroup itself- this is still a raw user pointer
    _SYSAssetGroup *userGroupPtr = reinterpret_cast<_SYSAssetGroup*>(header.pAssetGroup);
    _SYSAssetGroupCube *gc = cube.assetGroupCube(userGroupPtr);
    if (!gc)
        return false;
    baseAddr = gc->baseAddr;

    // Other member initialization
    blockCache.index = -1;

    return true;
}

unsigned ImageDecoder::getBlockSize() const
{
    switch (header.format) {
    
        case _SYS_AIF_DUB_I8:
        case _SYS_AIF_DUB_I16:
            return 8;

        default:
            // Unlimited
            return 0x7FFFFFFF;
    }
}

int ImageDecoder::tile(unsigned x, unsigned y, unsigned frame)
{
    if (x >= header.width || y >= header.height || frame >= header.frames)
        return NO_TILE;

    switch (header.format) {

        // Sequential tiles
        case _SYS_AIF_PINNED: {
            unsigned location = x + (y + frame * header.height) * header.width;
            return header.data + baseAddr + location;
        }

        // Uncompressed tile array
        case _SYS_AIF_FLAT: {
            unsigned location = x + (y + frame * header.height) * header.width;
            uint16_t tile;
            if (SvmMemory::copyROData(tile, header.data + (location << 1)))
                return tile + baseAddr;
            return NO_TILE;
        }

        // Compressed tile array, with 8x8x1 maximum block size
        case _SYS_AIF_DUB_I8:
        case _SYS_AIF_DUB_I16: {
            // Size of image, in blocks
            unsigned xBlocks = (header.width + 7) >> 3;
            unsigned yBlocks = (header.height + 7) >> 3;

            // Which block is this tile in?
            unsigned bx = x >> 3, by = y >> 3;
            unsigned blockNum = bx + (by + frame * yBlocks) * xBlocks;

            // How wide is the selected block?
            unsigned blockW = MIN(8, header.width - (x & ~7));

            if (blockCache.index != blockNum) {
                // This block isn't in the cache. Calculate the rest of its
                // size, and decompress it into the cache.

                unsigned blockH = MIN(8, header.height - (y & ~7));
                blockCache.index = blockNum;
                if (!decompressDUB(blockNum, blockW * blockH)) {
                    // Failure. Cache the failure, so we can fail fast!
                    for (unsigned i = 0; i < arraysize(blockCache.data); i++)
                        blockCache.data[i] = NO_TILE;
                }
            }

            return blockCache.data[(x & 7) + (y & 7) * blockW];
        }

        default: {
            return NO_TILE;
        }
    }
}

SvmMemory::VirtAddr ImageDecoder::readIndex(unsigned i)
{
    /*
     * Read a value from the format-specific index table.
     * Returns 0 on error.
     */

    switch (header.format) {

        // 8-bit index, relative to the next word address
        case _SYS_AIF_DUB_I8: {
            uint8_t value = 0;
            SvmMemory::VirtAddr va = header.data + i * sizeof(value);
            SvmMemory::VirtAddr wordVA = va & ~(SvmMemory::VirtAddr)1;
            if (SvmMemory::copyROData(ref, value, va))
                return wordVA + (value + 1) * sizeof(uint16_t);
            else
                return 0;
        }

        // 16-bit index, relative to the next word address
        case _SYS_AIF_DUB_I16: {
            uint16_t value = 0;
            SvmMemory::VirtAddr va = header.data + i * sizeof(value);
            if (SvmMemory::copyROData(ref, value, va))
                return va + (value + 1) * sizeof(uint16_t);
            else
                return 0;
        }

        default:
            return 0;
    }
}

bool ImageDecoder::decompressDUB(unsigned index, unsigned numTiles)
{
    DEBUG_LOG(("DUB[%08x]: Decompressing block %d, %d tiles\n",
        header.data, index, numTiles));

    struct Code {
        int type;
        int arg;
    };
    
    SvmMemory::VirtAddr va = readIndex(index);

    if (!va)
        return false;
    
    BitReader bits(ref, va);
    Code lastCode = { -1, 0 };
    uint16_t *tiles = blockCache.data;

    unsigned tileIndex = 0;
    for (;;) {
        /*
         * Read the next code
         */
        
        Code thisCode;
        thisCode.type = bits.read(1);

        if (thisCode.type) {
            // Backreference code
            thisCode.arg = bits.readVar();
            if ((unsigned)thisCode.arg >= tileIndex) {
                // Illegal backref
                return false;
            }
        } else {
            // Delta code
            bool sign = bits.read(1);
            int magnitude = bits.readVar();
            thisCode.arg = sign ? -magnitude : magnitude;
        }
        
        /*
         * Detect repeat codes. Any two consecutive identical codes
         * are followed by a repeat count.
         */

        unsigned repeats;
        if (thisCode.arg == lastCode.arg && thisCode.type == lastCode.type)
            repeats = bits.readVar();
        else
            repeats = 0;
        lastCode = thisCode;

        /*
         * Act on this code, possibly multiple times.
         */

        do {
            if (thisCode.type) {
                // Backreference, guaranteed to be valid
                tiles[tileIndex] = tiles[tileIndex - 1 - thisCode.arg];
            } else if (tileIndex) {
                // Delta from the prevous code
                tiles[tileIndex] = tiles[tileIndex - 1] + thisCode.arg;
            } else {
                // First tile, delta from baseAddr.
                tiles[tileIndex] = baseAddr + thisCode.arg;
            }

            tileIndex++;
            if (tileIndex == numTiles)
                return repeats == 0;
        
        } while (repeats--);
    }
}

unsigned BitReader::read(unsigned bits)
{
    /*
     * Read a fixed-width sequence of bits from the buffer, refilling it
     * from flash memory as necessary.
     */

    ASSERT(bits < 32);
    const unsigned mask = (1 << bits) - 1;

    for (;;) {
        
        // Have enough data?
        if (bits <= bitCount) {
            unsigned result = buffer & mask;
            buffer >>= bits;
            bitCount -= bits;
            return result;
        }

        // Buffer another 32 bits
        ASSERT(bitCount <= 32);
        uint32_t newBits;
        if (!SvmMemory::copyROData(ref, newBits, va))
            return 0;
        va += sizeof(newBits);
        buffer |= (buffer_t)newBits << bitCount;
        bitCount += 32;
    }
}

unsigned BitReader::readVar()
{
    /*
     * Read a variable-length integer.
     *
     * These integers consist of 3-bit chunks, each preceeded by a '1' bit.
     * The entire sequence is zero-terminated. Chunks are stored MSB-first.
     */

    const unsigned chunkSize = 3;
    unsigned result = 0;

    while (read(1)) {
        result <<= chunkSize;
        result |= read(chunkSize);
    }

    return result;
}
