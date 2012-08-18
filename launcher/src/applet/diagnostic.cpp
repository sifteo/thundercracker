/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "shared.h"
#include "diagnostic.h"
using namespace Sifteo;


void DiagnosticApplet::run()
{
    DiagnosticApplet instance;

    instance.install();

    // We're entirely event-driven
    while (1)
        System::paint();
}

void DiagnosticApplet::install()
{
    Events::neighborAdd.set(&DiagnosticApplet::onNeighborAdd, this);
    Events::neighborRemove.set(&DiagnosticApplet::onNeighborRemove, this);
    Events::cubeAccelChange.set(&DiagnosticApplet::onAccelChange, this);
    Events::cubeTouch.set(&DiagnosticApplet::onTouch, this);
    Events::cubeBatteryLevelChange.set(&DiagnosticApplet::onBatteryChange, this);
    Events::cubeConnect.set(&DiagnosticApplet::onConnect, this);

    // Handle already-connected cubes
    for (CubeID cube : CubeSet::connected())
        onConnect(cube);
}

void DiagnosticApplet::onConnect(unsigned id)
{
    CubeID cube(id);
    uint64_t hwid = cube.hwID();

    bzero(counters[id]);

    Shared::video[id].initMode(BG0_ROM);
    Shared::video[id].attach(id);
    Shared::motion[id].attach(id);

    // Draw the cube's identity
    String<128> str;
    str << "I am cube #" << cube << "\n";
    str << "hwid " << Hex(hwid >> 32) << "\n     " << Hex(hwid) << "\n\n";
    Shared::video[cube].bg0rom.text(vec(1,2), str);

    // Draw initial state for all sensors
    onAccelChange(cube);
    onBatteryChange(cube);
    onTouch(cube);
    drawNeighbors(cube);
}

void DiagnosticApplet::onBatteryChange(unsigned id)
{
    CubeID cube(id);
    String<32> str;
    str << "bat:   " << FixedFP(cube.batteryLevel(), 1, 3) << "\n";
    Shared::video[cube].bg0rom.text(vec(1,13), str);
}

void DiagnosticApplet::onTouch(unsigned id)
{
    CubeID cube(id);
    counters[id].touch++;

    String<32> str;
    str << "touch: " << cube.isTouching() <<
        " (" << counters[cube].touch << ")\n";
    Shared::video[cube].bg0rom.text(vec(1,9), str);
}

void DiagnosticApplet::onAccelChange(unsigned id)
{
    CubeID cube(id);
    auto accel = cube.accel();

    String<64> str;
    str << "acc: "
        << Fixed(accel.x, 3)
        << Fixed(accel.y, 3)
        << Fixed(accel.z, 3) << "\n";

    unsigned changeFlags = Shared::motion[id].update();
    if (changeFlags) {
        // Tilt/shake changed

        auto tilt = Shared::motion[id].tilt;
        str << "tilt:"
            << Fixed(tilt.x, 3)
            << Fixed(tilt.y, 3)
            << Fixed(tilt.z, 3) << "\n";

        str << "shake: " << Shared::motion[id].shake;
    }

    Shared::video[cube].bg0rom.text(vec(1,10), str);
}

void DiagnosticApplet::onNeighborRemove(unsigned firstID, unsigned firstSide, unsigned secondID, unsigned secondSide)
{
    if (firstID < arraysize(counters)) {
        counters[firstID].neighborRemove++;
        drawNeighbors(firstID);
    }
    if (secondID < arraysize(counters)) {
        counters[secondID].neighborRemove++;
        drawNeighbors(secondID);
    }
}

void DiagnosticApplet::onNeighborAdd(unsigned firstID, unsigned firstSide, unsigned secondID, unsigned secondSide)
{
    if (firstID < arraysize(counters)) {
        counters[firstID].neighborAdd++;
        drawNeighbors(firstID);
    }
    if (secondID < arraysize(counters)) {
        counters[secondID].neighborAdd++;
        drawNeighbors(secondID);
    }
}

void DiagnosticApplet::drawNeighbors(CubeID cube)
{
    Neighborhood nb(cube);

    String<64> str;
    str << "nb "
        << Hex(nb.neighborAt(TOP), 2) << " "
        << Hex(nb.neighborAt(LEFT), 2) << " "
        << Hex(nb.neighborAt(BOTTOM), 2) << " "
        << Hex(nb.neighborAt(RIGHT), 2) << "\n";

    str << "   +" << counters[cube].neighborAdd
        << ", -" << counters[cube].neighborRemove
        << "\n\n";

    BG0ROMDrawable &draw = Shared::video[cube].bg0rom;
    draw.text(vec(1,6), str);

    drawSideIndicator(draw, nb, vec( 1,  0), vec(14,  1), TOP);
    drawSideIndicator(draw, nb, vec( 0,  1), vec( 1, 14), LEFT);
    drawSideIndicator(draw, nb, vec( 1, 15), vec(14,  1), BOTTOM);
    drawSideIndicator(draw, nb, vec(15,  1), vec( 1, 14), RIGHT);
}

void DiagnosticApplet::drawSideIndicator(BG0ROMDrawable &draw, Neighborhood &nb,
    Int2 topLeft, Int2 size, Side s)
{
    unsigned nbColor = draw.ORANGE;
    draw.fill(topLeft, size,
        nbColor | (nb.hasNeighborAt(s) ? draw.SOLID_FG : draw.SOLID_BG));
}
