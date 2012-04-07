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
    int tile(unsigned x, unsigned y, unsigned frame);

    unsigned getWidth() const {
        return header.width;
    }

    unsigned getHeight() const {
        return header.height;
    }

    // Get the natural block size, used to generate a preferred access order
    unsigned getBlockSize() const;

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
