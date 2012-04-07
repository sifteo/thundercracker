/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_IMAGEDECODER_H
#define SVM_IMAGEDECODER_H

#include <sifteo/abi.h>
#include <stdint.h>
#include <inttypes.h>
#include "svmmemory.h"


/**
 * An ImageDecoder is designed to be a temporary object, constructed on the
 * stack, which handles format-specific decoding for _SYSAssetImage objects.
 *
 * We provide a random-access interface for fetching tiles from the asset,
 * with caching at the flash layer as well as in the decompression codec.
 */

class ImageDecoder {
public:
    static const int NO_TILE = -1;

    bool init(const _SYSAssetImage *userPtr, _SYSCubeID cid);
    bool init(const _SYSAssetImage *userPtr);

    int tile(unsigned x, unsigned y, unsigned frame);

    unsigned getWidth() const {
        return header.width;
    }

    unsigned getHeight() const {
        return header.height;
    }

    // Get a mask for bits in the address which refer to tiles within a block.
    // Other bits refer to the blocks themselves.
    uint16_t getBlockMask() const;

private:
    struct {
        uint16_t data[64];
        unsigned index;
    } blockCache;

    _SYSAssetImage header;
    uint16_t baseAddr;
    FlashBlockRef ref;

    bool decompressDUB(unsigned blockIndex, unsigned numTiles);
    SvmMemory::VirtAddr readIndex(unsigned i);
};


/**
 * An iterator for accessing tiles in an ImageDecoder in their natural order.
 * This iterates first over compression blocks, left-right top-bottom, then
 * over the individual tiles in a block. We can iterate on any subrectangle
 * of the image.
 */

class ImageIter {
public:
    ImageIter(ImageDecoder &decoder, unsigned frame, unsigned imageX, unsigned imageY,
        unsigned width, unsigned height)
        : decoder(decoder), x(imageX), y(imageY), left(imageX), top(imageY),
          right(imageX + width), bottom(imageY + height), frame(frame),
          blockMask(decoder.getBlockMask()) {}

    ImageIter(ImageDecoder &decoder, unsigned frame = 0)
        : decoder(decoder), x(0), y(0), left(0), top(0),
          right(decoder.getWidth()), bottom(decoder.getHeight()),
          frame(frame), blockMask(decoder.getBlockMask()) {}

    bool next() {
        {
            unsigned nextX = x + 1;                         // Next tile over within the block
            if ((nextX & blockMask) && nextX < right) {     // Still inside the block and image?
                x = nextX;                                  //   Yes, keep iterating horizontally.
                return true;
            }
        }

        // The rest of the work is handled by a non-inlined function...
        return nextWork();
    }

    int tile() const {
        return decoder.tile(x, y, frame);
    }

    uint16_t tile77() const {
        uint16_t t = decoder.tile(x, y, frame);
        return _SYS_TILE77(t);
    }

    uint16_t getWidth() const {
        return right - left;
    }

    uint16_t getHeight() const {
        return bottom - top;
    }
    
    uint16_t getImageX() const {
        return x;
    }

    uint16_t getImageY() const {
        return y;
    }

    uint16_t getRectX() const {
        return x - left;
    }

    uint16_t getRectY() const {
        return y - top;
    }

    unsigned getAddr(unsigned stride) const {
        return getRectX() + getRectY() * stride;
    }

    uint32_t getDestBytes(uint32_t stride) const;

    void copyToVRAM(_SYSVideoBuffer &vbuf, uint16_t originAddr, unsigned stride);
    void copyToMem(uint16_t *dest, unsigned stride);

private:
    ImageDecoder &decoder;

    uint16_t x;         // Current address within the image
    uint16_t y;
    uint16_t left;      // Iteration rectangle, in image coordinates
    uint16_t top;
    uint16_t right;
    uint16_t bottom;
    uint16_t frame;     // Frame to draw (constant)
    uint16_t blockMask; // Cached block mask for this image

    bool nextWork();
};


/**
 * This is a utility for reading streams of bits from flash.
 * We maintain a 64-bit shift register, and refill the upper
 * 32 bits when it runs empty.
 */

class BitReader {
public:
    BitReader(FlashBlockRef &ref, SvmMemory::VirtAddr va)
        : buffer(0), bitCount(0), ref(ref), va(va) {}

    unsigned read(unsigned bits);
    unsigned readVar();

private:
    typedef uint64_t buffer_t;
    buffer_t buffer;
    unsigned bitCount;
    FlashBlockRef &ref;
    SvmMemory::VirtAddr va;
};


#endif // SVM_IMAGEDECODER_H
