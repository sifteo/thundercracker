/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>

class DefaultCubeResponder
{
public:
    DefaultCubeResponder() : needInit(true) { cube = Sifteo::CubeID(); }
    void init(Sifteo::CubeID cid);
    void init() { needInit = true; }
    void paint();

    static unsigned callCount() { return called; }
    static void resetCallCount() { called = 0; }

private:
    static const Sifteo::Float2 kRest;
    static const float kRate;
    static const float kMag;
    static const float kShakeCount;
    static const float kDownTarget;
    static const float kDownRate;
    static const float kUpRate;

    void _init();

    Sifteo::CubeID cube;

    float u;
    Sifteo::Float2 position;

    bool needInit;
    static unsigned called;
};
