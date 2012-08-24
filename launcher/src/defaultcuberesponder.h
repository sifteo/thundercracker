/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>

class DefaultCubeResponder
{
public:
    void init(Sifteo::CubeID cid);
    void init();
    void paint();

    static unsigned callCount() { return called; }
    static void resetCallCount() { called = 0; }

private:
    Sifteo::Short2 position;
    Sifteo::Short2 velocity;
    Sifteo::CubeID cube;
    bool wasTouching;

    static unsigned called;

    void motionUpdate();

    Sifteo::Int2 fpRound(Sifteo::Int2 fp, unsigned bits) {
        return (fp + (Sifteo::vec(1,1) << (bits-1))) >> bits;
    }

    Sifteo::Int2 fpTrunc(Sifteo::Int2 fp, unsigned bits) {
        return fp >> bits;
    }
};
