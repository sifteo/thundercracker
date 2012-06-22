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

    static ALWAYS_INLINE void Barrier() {
        __sync_synchronize();
    }
    
    static ALWAYS_INLINE void Add(uint32_t &dest, uint32_t src) {
        __sync_add_and_fetch(&dest, src);
    }

    static ALWAYS_INLINE void Add(int32_t &dest, int32_t src) {
        __sync_add_and_fetch(&dest, src);
    }
    
    static ALWAYS_INLINE void Or(uint32_t &dest, uint32_t src) {
        __sync_or_and_fetch(&dest, src);
    }

    static ALWAYS_INLINE void And(uint32_t &dest, uint32_t src) {
        __sync_and_and_fetch(&dest, src);
    }

    static ALWAYS_INLINE void Store(uint32_t &dest, uint32_t src) {
        Barrier();
        dest = src;
        Barrier();
    }

    static ALWAYS_INLINE void Store(int32_t &dest, int32_t src) {
        Barrier();
        dest = src;
        Barrier();
    }

    static ALWAYS_INLINE uint32_t Load(uint32_t &src) {
        Barrier();
        uint32_t dest = src;
        Barrier();
        return dest;
    }

    static ALWAYS_INLINE int32_t Load(int32_t &src) {
        Barrier();
        int32_t dest = src;
        Barrier();
        return dest;
    }

    /*
     * XXX: Bit operations should be implemented using the Bit Band on Cortex-M3.
     */

    static ALWAYS_INLINE void SetBit(uint32_t &dest, unsigned bit) {
        ASSERT(bit < 32);
        Or(dest, 1 << bit);
    }

    static ALWAYS_INLINE void ClearBit(uint32_t &dest, unsigned bit) {
        ASSERT(bit < 32);
        And(dest, ~(1 << bit));
    }

    static ALWAYS_INLINE void SetLZ(uint32_t &dest, unsigned bit) {
        ASSERT(bit < 32);
        Or(dest, 0x80000000 >> bit);
    }

    static ALWAYS_INLINE void ClearLZ(uint32_t &dest, unsigned bit) {
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
	
	static ALWAYS_INLINE uint32_t POPCOUNT(uint32_t i) {
        // Returns the number of 1-bits in i.
        return __builtin_popcount(i);
    }

    static ALWAYS_INLINE uint32_t CLZ(uint32_t r) {
        // Count leading zeroes. One instruction on ARM.
        return __builtin_clz(r);
    }

    static ALWAYS_INLINE uint32_t LZ(uint32_t l) {
        // Generate number with 'l' leading zeroes. Inverse of CLZ.
        ASSERT(l < 32);
        return 0x80000000 >> l;
    }

    static ALWAYS_INLINE uint32_t CTZ(uint32_t r) {
        // Count trailing zeroes. Reference:
        // http://en.wikipedia.org/wiki/Find_first_set#Properties_and_relations
        return 31 - CLZ(r & -r);
    }

    static ALWAYS_INLINE uint32_t ROR(uint32_t a, uint32_t b) {
        // Rotate right. One instruction on ARM
        if (b < 32)
            return (a >> b) | (a << (32 - b));
        else
            return 0;
    }

    static ALWAYS_INLINE uint32_t ROL(uint32_t a, uint32_t b) {
        return ROR(a, 32 - b);
    }

    // Clamps 'r' to the range [-2^(bit-1), 2^(bit-1) - 1]
    static ALWAYS_INLINE int32_t SSAT(int32_t r, unsigned bit) {
        #ifdef SIFTEO_SIMULATOR
            r = MIN(r, (1 << (bit-1)) - 1);
            r = MAX(r, -(1 << (bit-1)));
            return r;
        #else
            int32_t dest;
            asm volatile (
                "ssat %[dest], %[imm], %[src]"
                : [dest] "=r"(dest)
                : [imm] "i"(bit), [src] "r"(r)
                : "cc");
            return dest;
        #endif
    }

} // namespace Intrinsic


#endif
