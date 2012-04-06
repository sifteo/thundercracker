/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _BITS_H
#define _BITS_H

#include <assert.h>
#include <stdint.h>
#include <vector>

namespace Stir {


/**
 * A BitBuffer is a vector of bits, capable of holding up to 32 bits
 * at a time. Accumulated bytes or words are flushed to a std::vector.
 *
 * All values here are assumed to be little-endian.
 */

class BitBuffer {
 public:
    BitBuffer() : bits(0), count(0) {}

    template <typename T> void flush(std::vector<T> &buf, bool pad=false) {
        const unsigned bitWidth = sizeof(T) * 8;
        assert(count <= sizeof(bits)*8);
        assert(bitWidth < sizeof(bits)*8);

        while (count > bitWidth) {
            buf.push_back((T) bits);
            count -= bitWidth;
            bits >>= bitWidth;
        }

        if (pad && count != 0) {
            buf.push_back((T) bits);
            count = 0;
            bits = 0;
        }
    }

    void append(uint32_t value, unsigned width) {
        // Overflow-safe asserts
        assert(count <= sizeof(bits)*8);
        assert(width <= sizeof(bits)*8);

        // Mask to 'width'
        value = value & ((1 << width) - 1);

        bits |= value << count;
        count += width;
        assert(count <= sizeof(bits)*8);
    }

    unsigned countChunks(uint32_t value, unsigned chunkSize) {
        unsigned result = 0;
        while (value) {
            value >>= chunkSize;
            result++;
        }
        return result;
    }

    void appendVar(uint32_t value, unsigned chunkSize) {
        // Append a variable-width integer. We output 'chunkSize'
        // bits at a time, MSB first. Each chunk is preceeded by a '1' bit,
        // and the whole sequence is zero-terminated.

        for (unsigned chunk = 0, numChunks = countChunks(value, chunkSize);
            chunk != numChunks; chunk++) {

            unsigned shift = (numChunks - 1 - chunk) * chunkSize;
            append(1, 1);
            append(value >> shift, chunkSize);
        }
        append(0, 1);

    }

    unsigned getCount() const {
        return count;
    }
    
    uint64_t getBits() const {
        return bits;
    }

 private:
    uint64_t bits;
    unsigned count;
};


};  // namespace Stir

#endif
