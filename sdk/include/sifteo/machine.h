/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_MACHINE_H
#define _SIFTEO_MACHINE_H

#include <stdint.h>

/*
 * Machine-specific utility definitions
 */

#ifndef MIN
#define MIN(a,b)   ((a) < (b) ? (a) : (b))
#define MAX(a,b)   ((a) > (b) ? (a) : (b))
#endif

#ifndef arraysize
#define arraysize(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef offsetof
#define offsetof(t,m)  ((uintptr_t)(uint8_t*)&(((t*)0)->m))
#endif

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
	__asm__ __volatile__ ("" : : : "memory");
    }
    
    static inline void Or(uint32_t &dest, uint32_t src) {
	Barrier();
	dest |= src;
	Barrier();
    }

    static inline void And(uint32_t &dest, uint32_t src) {
	Barrier();
	dest &= src;
	Barrier();
    }

};


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
	
	uint32_t c;
	for (c = 0; c < 32; c++) {
	    if (r & 0x80000000)
		break;
	    else
		r <<= 1;
	}
	return c;
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

};


};  // namespace Sifteo

#endif
