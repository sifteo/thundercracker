/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef BITS_H
#define BITS_H

#include "macros.h"
#include "machine.h"
#include "crc.h"


/**
 * A vector of bits which uses Intrinsic::CLZ to support fast iteration.
 *
 * Supports vectors with any fixed-size number of bits. For sizes <= 32 bits,
 * this is just as efficient as using a bare uint32_t.
 */

template <unsigned tSize>
class BitVector
{
public:
    uint32_t words[(tSize + 31) / 32];

    /// Mark (set to 1) a single bit
    void mark(unsigned index)
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        ASSERT(index < tSize);
        if (NUM_WORDS > 1) {
            unsigned word = index >> 5;
            unsigned bit = index & 31;
            words[word] |= Intrinsic::LZ(bit);
        } else {
            words[0] |= Intrinsic::LZ(index);
        }
    }

    /// Clear (set to 0) a single bit
    void clear(unsigned index)
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        ASSERT(index < tSize);
        if (NUM_WORDS > 1) {
            unsigned word = index >> 5;
            unsigned bit = index & 31;
            words[word] &= ~Intrinsic::LZ(bit);
        } else {
            words[0] &= ~Intrinsic::LZ(index);
        }
    }

    /// Mark (set to 1) all bits in the vector
    void mark()
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;
        const unsigned NUM_FULL_WORDS = tSize / 32;
        const unsigned REMAINDER_BITS = tSize & 31;

        STATIC_ASSERT(NUM_FULL_WORDS + 1 == NUM_WORDS ||
                      NUM_FULL_WORDS == NUM_WORDS);

        // Set fully-utilized words only
        for (unsigned i = 0; i < NUM_FULL_WORDS; ++i)
            words[i] = -1;

        if (NUM_FULL_WORDS != NUM_WORDS) {
            // Set only bits < tSize in the last word.
            uint32_t mask = ((uint32_t)-1) << ((32 - REMAINDER_BITS) & 31);
            words[NUM_FULL_WORDS] = mask;
        }
    }

    /// Clear (set to 0) all bits in the vector
    void clear()
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        for (unsigned i = 0; i < NUM_WORDS; ++i)
            words[i] = 0;
    }

    /// Is a particular bit marked?
    bool test(unsigned index)
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        ASSERT(index < tSize);
        if (NUM_WORDS > 1) {
            unsigned word = index >> 5;
            unsigned bit = index & 31;
            return (words[word] & Intrinsic::LZ(bit)) != 0;
        } else {
            return (words[0] & Intrinsic::LZ(index)) != 0;
        }
    }

    /// Is every bit in this vector set to zero?
    bool empty() const
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        if (NUM_WORDS > 1) {
            for (unsigned w = 0; w < NUM_WORDS; w++)
                if (words[w])
                    return false;
            return true;
        } else {
            return words[0] == 0;
        }
    }

    /**
     * Find the lowest index where there's a marked (1) bit.
     * If any marked bits exist, returns true and puts the bit's index
     * in "index". Iff the entire vector is zero, returns false.
     */
    bool findFirst(unsigned &index)
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        if (NUM_WORDS > 1) {
            for (unsigned w = 0; w < NUM_WORDS; w++) {
                uint32_t v = words[w];
                if (v) {
                    index = (w << 5) | Intrinsic::CLZ(v);
                    ASSERT(index < tSize);
                    return true;
                }
            }
        } else {
            uint32_t v = words[0];
            if (v) {
                index = Intrinsic::CLZ(v);
                ASSERT(index < tSize);
                return true;
            }
        }
        return false;
    }

    /**
     * Find the lowest marked bit. If we find any marked bits,
     * returns true, sets "index" to that bit's index, and clears the
     * bit. This can be used as part of an iteration pattern:
     *
     *       unsigned index;
     *       while (vec.clearFirst(index)) {
     *           doStuff(index);
     *       }
     *
     * This is functionally equivalent to findFirst() followed by
     * clear(), but it's a tiny bit more efficient.
     */
    bool clearFirst(unsigned &index)
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        if (NUM_WORDS > 1) {
            for (unsigned w = 0; w < NUM_WORDS; w++) {
                uint32_t v = words[w];
                if (v) {
                    unsigned bit = Intrinsic::CLZ(v);
                    words[w] ^= Intrinsic::LZ(bit);
                    index = (w << 5) | bit;
                    ASSERT(index < tSize);
                    return true;
                }
            }
        } else {
            uint32_t v = words[0];
            if (v) {
                unsigned bit = Intrinsic::CLZ(v);
                words[0] ^= Intrinsic::LZ(bit);
                index = bit;
                ASSERT(index < tSize);
                return true;
            }
        }
        return false;
    }
};


/**
 * 8-bit right rotate
 */
inline uint8_t ROR8(uint8_t a, unsigned shift)
{
    return (a << (8 - shift)) | (a >> shift);
}


static uint8_t computeCheckByte(uint8_t a, uint8_t b);



#endif
