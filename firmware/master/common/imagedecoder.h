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

private:
    struct {
        uint16_t data[64];
        unsigned index;
    } blockCache;

    _SYSAssetImage header;
    uint16_t baseAddr;
    FlashBlockRef ref;

    void decompressDUB(unsigned blockIndex);
};


#endif // SVM_IMAGEDECODER_H
