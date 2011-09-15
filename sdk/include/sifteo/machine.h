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

namespace Sifteo {


/**
 * Atomic operations 
 *
 * XXX: Implement these for each supported platform
 */

class Atomic {
 public:
    
    static void Or(uint32_t &dest, uint32_t src) {
	dest |= src;
    }

    static void And(uint32_t &dest, uint32_t src) {
	dest &= src;
    }

};


};  // namespace Sifteo

#endif
