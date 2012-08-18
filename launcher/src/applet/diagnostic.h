/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>


class DiagnosticApplet {
public:
	static void run();

private:
    struct Counter {
        unsigned touch;
        unsigned neighborAdd;
        unsigned neighborRemove;
    } counters[CUBE_ALLOCATION];

    DiagnosticApplet() {}

    void install();

    void onConnect(unsigned id);
    void onBatteryChange(unsigned id);
    void onTouch(unsigned id);
    void onAccelChange(unsigned id);
    void onNeighborRemove(unsigned firstID, unsigned firstSide, unsigned secondID, unsigned secondSide);
    void onNeighborAdd(unsigned firstID, unsigned firstSide, unsigned secondID, unsigned secondSide);
    void drawNeighbors(Sifteo::CubeID cube);

    static void drawSideIndicator(Sifteo::BG0ROMDrawable &draw, Sifteo::Neighborhood &nb,
        Sifteo::Int2 topLeft, Sifteo::Int2 size, Sifteo::Side s);
};
