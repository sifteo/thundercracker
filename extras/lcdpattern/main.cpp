/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static Metadata M = Metadata()
    .title("Sensors LCD pattern test")
    .package("com.sifteo.sdk.lcdpattern", "1.0")
    .icon(Icon)
    .cubeRange(0, CUBE_ALLOCATION);

static VideoBuffer vid[CUBE_ALLOCATION];

class Listener {
public:
    void install()
    {
        Events::cubeConnect.set(&Listener::onConnect, this);

        // Handle already-connected cubes
        for (CubeID cube : CubeSet::connected())
            onConnect(cube);
    }

private:
    void onConnect(unsigned id)
    {
        CubeID cube(id);
        LOG("Cube %d connected\n", id);

        vid[id].initMode(BG0_ROM);
        vid[id].attach(id);

        // Draw the pattern
        BG0ROMDrawable &draw = vid[id].bg0rom;
        unsigned colorTL = draw.RED;
        unsigned colorTR = draw.GREEN;
        unsigned colorBL = draw.BLUE;
        unsigned colorBR = draw.BLACK;
        draw.fill(vec(1,1), vec(7,7), colorTL | draw.SOLID_FG);
        draw.fill(vec(8,1), vec(7,7), colorTR | draw.SOLID_FG);
        draw.fill(vec(1,8), vec(7,7), colorBL | draw.SOLID_FG);
        draw.fill(vec(8,8), vec(7,7), colorBR | draw.SOLID_FG);
    }
};


void main()
{ 
    static Listener lcdpattern;

    lcdpattern.install();

    // We're entirely event-driven.
    while (1)
        System::paint();
}
