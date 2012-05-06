/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef MACHINE_H
#define MACHINE_H

#include "macros.h"


/**
 * Atomic operations.
 *
 * Guaranteed memory atomicity (single-instruction read-modify-write)
 * and write ordering.
 */

namespace Atomic {

    static inline void Barrier() {
        __sync_synchronize();
    }
    
    static inline void Add(uint32_t &dest, uint32_t src) {
        __sync_add_and_fetch(&dest, src);
    }

    static inline void Add(int32_t &dest, int32_t src) {
        __sync_add_and_fetch(&dest, src);
    }
    
    static inline void Or(uint32_t &dest, uint32_t src) {
        __sync_or_and_fetch(&dest, src);
    }

    static inline void And(uint32_t &dest, uint32_t src) {
        __sync_and_and_fetch(&dest, src);
    }

    static inline void Store(uint32_t &dest, uint32_t src) {
        Barrier();
        dest = src;
        Barrier();
    }

    static inline void Store(int32_t &dest, int32_t src) {
        Barrier();
        dest = src;
        Barrier();
    }

    static inline uint32_t Load(uint32_t &src) {
        Barrier();
        uint32_t dest = src;
        Barrier();
        return dest;
    }

    static inline int32_t Load(int32_t &src) {
        Barrier();
        int32_t dest = src;
        Barrier();
        return dest;
    }

    /*
     * XXX: Bit operations should be implemented using the Bit Band on Cortex-M3.
     */

    static inline void SetBit(uint32_t &dest, unsigned bit) {
        ASSERT(bit < 32);
        Or(dest, 1 << bit);
    }

    static inline void ClearBit(uint32_t &dest, unsigned bit) {
        ASSERT(bit < 32);
        And(dest, ~(1 << bit));
    }

    static inline void SetLZ(uint32_t &dest, unsigned bit) {
        ASSERT(bit < 32);
        Or(dest, 0x80000000 >> bit);
    }

    static inline void ClearLZ(uint32_t &dest, unsigned bit) {
        ASSERT(bit < 32);
        And(dest, ~(0x80000000 >> bit));
    }

} // namespace Atomic


/**
 * Assembly intrinsics
 *
 * These functions basic operations that our hardware can do very
 * fast, but for with no analog exists in C.
 */

namespace Intrinsic {
	
	static inline uint32_t POPCOUNT(uint32_t i) {
        // Returns the number of 1-bits in i.
        return __builtin_popcount(i);
    }

    static inline uint32_t CLZ(uint32_t r) {
        // Count leading zeroes. One instruction on ARM.
        return __builtin_clz(r);
    }

    static inline uint32_t LZ(uint32_t l) {
        // Generate number with 'l' leading zeroes. Inverse of CLZ.
        ASSERT(l < 32);
        return 0x80000000 >> l;
    }

    static inline uint32_t ROR(uint32_t a, uint32_t b) {
        // Rotate right. One instruction on ARM
        if (b < 32)
            return (a >> b) | (a << (32 - b));
        else
            return 0;
    }

    static inline uint32_t ROL(uint32_t a, uint32_t b) {
        return ROR(a, 32 - b);
    }

} // namespace Intrinsic


/**
 * A vector of bits which uses Intrinsic::CLZ to support fast iteration.
 *
 * Supports vectors with any fixed-size number of bits. For sizes <= 32 bits,
 * this is just as efficient as using a bare uint32_t.
 */

template <unsigned tSize>
class BitVector
{
private:
    static const unsigned NUM_WORDS = (tSize + 31) / 32;
    static const unsigned NUM_FULL_WORDS = tSize / 32;
    static const unsigned REMAINDER_BITS = tSize & 31;

    uint32_t words[NUM_WORDS];

public:
    /// Mark (set to 1) a single bit
    void mark(unsigned index) {
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
    void clear(unsigned index) {
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
    void mark() {
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
    void clear() {
        for (unsigned i = 0; i < NUM_WORDS; ++i)
            words[i] = 0;
    }

    /// Is a particular bit marked?
    bool test(unsigned index) {
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
    bool empty() const {
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
    bool findFirst(unsigned &index) {
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
    bool clearFirst(unsigned &index) {
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


#endif
