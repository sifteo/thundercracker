/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
using namespace Sifteo;

static Metadata M = Metadata()
    .title("Sensors SDK Example");


class EventCounters {
public:
    struct {
        unsigned touch;
        unsigned shake;
        unsigned neighborAdd;
        unsigned neighborRemove;
    } cubes[CUBE_ALLOCATION];
    
    void install()
    {
        Events::cubeTouch.set(&EventCounters::onTouch, this);
        Events::cubeShake.set(&EventCounters::onShake, this);
        Events::neighborAdd.set(&EventCounters::onNeighborAdd, this);
        Events::neighborRemove.set(&EventCounters::onNeighborRemove, this);
    }

private:
    void onTouch(unsigned cube)
    {
        cubes[cube].touch++;
        LOG(("Touched cube #%d\n", cube));
    }

    void onShake(unsigned cube)
    {
        cubes[cube].shake++;
        LOG(("Shaking cube #%d\n", cube));
    }

    void onNeighborRemove(unsigned firstCube, unsigned firstSide,
        unsigned secondCube, unsigned secondSide)
    {
        cubes[firstCube].neighborRemove++;
        cubes[secondCube].neighborRemove++;
        LOG(("Neighbor Remove: %d:%d - %d:%d\n",
            firstCube, firstSide, secondCube, secondSide));
    }

    void onNeighborAdd(unsigned firstCube, unsigned firstSide,
        unsigned secondCube, unsigned secondSide)
    {
        cubes[firstCube].neighborAdd++;
        cubes[secondCube].neighborAdd++;
        LOG(("Neighbor Add: %d:%d - %d:%d\n",
            firstCube, firstSide, secondCube, secondSide));
    }
};


void main()
{
    static const unsigned numCubes = 3;
    static VideoBuffer vid[CUBE_ALLOCATION];
    static EventCounters counters;
    counters.install();

    for (CubeID cube = 0; cube < numCubes; ++cube) {
        cube.enable();
        vid[cube].initMode(BG0_ROM);
        vid[cube].attach(cube);
    }

    while (1) {
        for (CubeID cube = 0; cube < numCubes; ++cube) {
            auto &draw = vid[cube].bg0rom;
            String<192> str;

            /*
             * Textual dump of current sensor state
             */

            uint64_t hwid = cube.hwID();
            str << "I am cube #" << cube << "\n";
            str << "hwid " << Hex(hwid >> 32) << "\n     " << Hex(hwid) << "\n\n";

            Neighborhood nb(cube);
            str << "nb "
                << Hex(nb.neighborAt(TOP), 2) << " "
                << Hex(nb.neighborAt(LEFT), 2) << " "
                << Hex(nb.neighborAt(BOTTOM), 2) << " "
                << Hex(nb.neighborAt(RIGHT), 2) << "\n";

            str << "   +" << counters.cubes[cube].neighborAdd
                << ", -" << counters.cubes[cube].neighborRemove
                << "\n\n";

            str << "bat:   " << Hex(cube.batteryLevel(), 4) << "\n";
            str << "touch: " << cube.isTouching() <<
                " (" << counters.cubes[cube].touch << ")\n";

            auto accel = cube.accel();
            str << "acc: "
                << Fixed(accel.x, 3)
                << Fixed(accel.y, 3)
                << Fixed(accel.z, 3) << "\n";

            auto tilt = cube.tilt();
            str << "tilt:"
                << Fixed(tilt.x, 3)
                << Fixed(tilt.y, 3) << "\n";

            str << "shake: " << counters.cubes[cube].shake;

            draw.text(Vec2(1,2), str);

            /*
             * Neighboring indicator bars
             */

            unsigned nbColor = draw.ORANGE;

            draw.fill(Vec2(1, 0), Vec2(14, 1),
                nbColor | (nb.hasNeighborAt(TOP) ? draw.SOLID_FG : draw.SOLID_BG));
            draw.fill(Vec2(0, 1), Vec2(1, 14),
                nbColor | (nb.hasNeighborAt(LEFT) ? draw.SOLID_FG : draw.SOLID_BG));
            draw.fill(Vec2(1, 15), Vec2(14, 1),
                nbColor | (nb.hasNeighborAt(BOTTOM) ? draw.SOLID_FG : draw.SOLID_BG));
            draw.fill(Vec2(15, 1), Vec2(1, 14),
                nbColor | (nb.hasNeighborAt(RIGHT) ? draw.SOLID_FG : draw.SOLID_BG));
        }

        System::paint();
    }
}
