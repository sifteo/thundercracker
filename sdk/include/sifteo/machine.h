/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_MACHINE_H
#define _SIFTEO_MACHINE_H

#include <stdint.h>
#include <sifteo/macros.h>

namespace Sifteo {

/**
 * Atomic operations.
 *
 * Guaranteed memory atomicity (single-instruction read-modify-write)
 * and write ordering.
 *
 * XXX: Implement these for each supported platform
 */

namespace Atomic {

    static inline void Barrier() {
#ifdef __GNUC__
        __sync_synchronize();
#endif
    }
    
    static inline void Add(uint32_t &dest, uint32_t src) {
#ifdef __GNUC__
        __sync_add_and_fetch(&dest, src);
#else
        dest += src;
#endif
    }

    static inline void Add(int32_t &dest, int32_t src) {
#ifdef __GNUC__
        __sync_add_and_fetch(&dest, src);
#else
        dest += src;
#endif
    }
    
    static inline void Or(uint32_t &dest, uint32_t src) {
#ifdef __GNUC__
        __sync_or_and_fetch(&dest, src);
#else
        dest |= src;
#endif
    }

    static inline void And(uint32_t &dest, uint32_t src) {
#ifdef __GNUC__
        __sync_and_and_fetch(&dest, src);
#else
        dest &= src;
#endif
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
 *
 * XXX: Implement these for each supported platform
 */

namespace Intrinsic {

    static inline uint32_t CLZ(uint32_t r) {
        // Count leading zeroes. One instruction on ARM.
#ifdef __GNUC__
        return __builtin_clz(r);
#else   
        uint32_t c;
        for (c = 0; c < 32; c++) {
            if (r & 0x80000000)
                break;
            else
                r <<= 1;
        }
        return c;
#endif
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


}  // namespace Sifteo

#endif
