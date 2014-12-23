/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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

#ifndef SVM_IMAGEDECODER_H
#define SVM_IMAGEDECODER_H

#include <sifteo/abi.h>
#include "macros.h"
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

    ALWAYS_INLINE unsigned getWidth() const {
        return header.width;
    }

    ALWAYS_INLINE unsigned getHeight() const {
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

    ALWAYS_INLINE void reset() {
        x = left;
        y = top;
    }

    ALWAYS_INLINE bool next() {
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

    ALWAYS_INLINE int tile() const {
        return decoder.tile(x, y, frame);
    }

    ALWAYS_INLINE uint16_t tile77() const {
        uint16_t t = decoder.tile(x, y, frame);
        return _SYS_TILE77(t);
    }

    ALWAYS_INLINE uint16_t getWidth() const {
        return right - left;
    }

    ALWAYS_INLINE uint16_t getHeight() const {
        return bottom - top;
    }
    
    ALWAYS_INLINE uint16_t getImageX() const {
        return x;
    }

    ALWAYS_INLINE uint16_t getImageY() const {
        return y;
    }

    ALWAYS_INLINE uint16_t getRectX() const {
        return x - left;
    }

    ALWAYS_INLINE uint16_t getRectY() const {
        return y - top;
    }

    ALWAYS_INLINE unsigned getAddr(unsigned stride) const {
        return getRectX() + getRectY() * stride;
    }

    uint32_t getDestBytes(uint32_t stride) const;

    void copyToVRAM(_SYSVideoBuffer &vbuf, uint16_t originAddr, unsigned stride);
    void copyToBG1(_SYSVideoBuffer &vbuf, unsigned destX, unsigned destY);
    void copyToBG1Masked(_SYSVideoBuffer &vbuf, uint16_t key);
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
