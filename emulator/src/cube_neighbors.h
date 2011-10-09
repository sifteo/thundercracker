/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_NEIGHBORS_H
#define _CUBE_NEIGHBORS_H

#include <stdint.h>

#include "vtime.h"

namespace Cube {


class Neighbors {
 public:
    enum Side {
        TOP = 0,
        RIGHT,
        BOTTOM,
        LEFT,
        NUM_SIDES,
    };

    void init() {
        memset(&mySides, 0, sizeof mySides);
    };
    
    void setContact(unsigned mySide, unsigned otherSide, unsigned otherCube) {
        /* Mark two neighbor sensors as in-range. ONLY called by the UI thread. */
        mySides[mySide].otherSides[otherSide] |= 1 << otherCube;
    }

    void clearContact(unsigned mySide, unsigned otherSide, unsigned otherCube) {
        /* Mark two neighbor sensors as not-in-range. ONLY called by the UI thread. */
        mySides[mySide].otherSides[otherSide] &= ~(1 << otherCube);
    }

 private:
    struct {
        uint32_t otherSides[NUM_SIDES];
    } mySides[NUM_SIDES];
};


};  // namespace Cube

#endif
