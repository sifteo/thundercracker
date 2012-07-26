/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static VideoBuffer vid[CUBE_ALLOCATION];

static Metadata M = Metadata()
    .title("Sensors SDK Example")
    .package("com.sifteo.sdk.sensors", "1.0")
    .icon(Icon)
    .cubeRange(0, CUBE_ALLOCATION);

static void updateNeighbors(unsigned firstCube, unsigned secondCube);


class EventCounters {
public:
    struct CubeInfo {
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
        Events::cubeAccelChange.set(&EventCounters::onAccelChange, this);
        Events::cubeConnect.set(&EventCounters::onConnect, this);
    }

private:
    void onConnect(unsigned id)
    {
        LOG("Cube %d connected\n", id);
        bzero(cubes[id]);
        vid[id].initMode(BG0_ROM);
        vid[id].attach(id);
    }

    void onTouch(unsigned id)
    {
        CubeID cube(id);
        cubes[id].touch++;
        LOG("Touched cube #%d\n", id);

        String<32> str;
        str << "touch: " << cube.isTouching() <<
            " (" << cubes[cube].touch << ")\n";
        vid[cube].bg0rom.text(vec(1,8), str);
    }

    void onShake(unsigned cube)
    {
        cubes[cube].shake++;
        LOG("Shaking cube #%d\n", cube);

        String<16> str;
        str << "shake: " << cubes[cube].shake;
        vid[cube].bg0rom.text(vec(1,12), str);
    }

    void onAccelChange(unsigned id)
    {
        CubeID cube(id);
        auto accel = cube.accel();

        String<64> str;
        str << "acc: "
            << Fixed(accel.x, 3)
            << Fixed(accel.y, 3)
            << Fixed(accel.z, 3) << "\n";

        auto tilt = cube.tilt();
        str << "tilt:"
            << Fixed(tilt.x, 3)
            << Fixed(tilt.y, 3)
            << Fixed(tilt.z, 3) << "\n";
        vid[cube].bg0rom.text(vec(1,10), str);
    }

    void onNeighborRemove(unsigned firstCube, unsigned firstSide,
        unsigned secondCube, unsigned secondSide)
    {
        cubes[firstCube].neighborRemove++;
        cubes[secondCube].neighborRemove++;
        LOG("Neighbor Remove: %d:%d - %d:%d\n",
            firstCube, firstSide, secondCube, secondSide);
        updateNeighbors(firstCube, secondCube);
    }

    void onNeighborAdd(unsigned firstCube, unsigned firstSide,
        unsigned secondCube, unsigned secondSide)
    {
        cubes[firstCube].neighborAdd++;
        cubes[secondCube].neighborAdd++;
        LOG("Neighbor Add: %d:%d - %d:%d\n",
            firstCube, firstSide, secondCube, secondSide);
        updateNeighbors(firstCube, secondCube);
    }
};

static EventCounters counters;

static void drawSideIndicator(BG0ROMDrawable &draw, Neighborhood &nb,
    Int2 topLeft, Int2 size, Side s)
{
    unsigned nbColor = draw.ORANGE;
    draw.fill(topLeft, size,
        nbColor | (nb.hasNeighborAt(s) ? draw.SOLID_FG : draw.SOLID_BG));
}

void updateNeighbors(unsigned firstCube, unsigned secondCube) {
    /*
     * Neighboring indicator bars
     */
    const unsigned neighborCubes[] = { firstCube, secondCube };
    for (unsigned i = 0; i < 2; ++i) {
        CubeID cube = neighborCubes[i];
        Neighborhood nb(cube);

        String<64> str;
        str << "nb "
            << Hex(nb.neighborAt(TOP), 2) << " "
            << Hex(nb.neighborAt(LEFT), 2) << " "
            << Hex(nb.neighborAt(BOTTOM), 2) << " "
            << Hex(nb.neighborAt(RIGHT), 2) << "\n";

        str << "   +" << counters.cubes[cube].neighborAdd
            << ", -" << counters.cubes[cube].neighborRemove
            << "\n\n";

        BG0ROMDrawable &draw = vid[cube].bg0rom;
        draw.text(vec(1,6), str);

        drawSideIndicator(draw, nb, vec( 1,  0), vec(14,  1), TOP);
        drawSideIndicator(draw, nb, vec( 0,  1), vec( 1, 14), LEFT);
        drawSideIndicator(draw, nb, vec( 1, 15), vec(14,  1), BOTTOM);
        drawSideIndicator(draw, nb, vec(15,  1), vec( 1, 14), RIGHT);
    }
}

void main()
{
    counters.install();
    uint64_t hwids[CUBE_ALLOCATION];
    unsigned batteryLvls[CUBE_ALLOCATION];

    for (CubeID cube : CubeSet::connected()) {
        vid[cube].initMode(BG0_ROM);
        vid[cube].attach(cube);
        hwids[cube] = _SYS_INVALID_HWID;
        batteryLvls[cube] = 0;
    }

    while (1) {
       for (CubeID cube : CubeSet::connected()) {
            /*
             * Draw each cube's hardware ID.
             * XXX: this can be event driven once we get cube
             *      connect/disconnect events.
             */
            if (hwids[cube] == _SYS_INVALID_HWID) {
                uint64_t hwid = cube.hwID();
                hwids[cube] = hwid;

                String<128> str;
                str << "I am cube #" << cube << "\n";
                str << "hwid " << Hex(hwid >> 32) << "\n     " << Hex(hwid) << "\n\n";
                vid[cube].bg0rom.text(vec(1,2), str);
            }

            /*
             * Poll for battery changes.
             */
            const unsigned lvl = cube.batteryLevel();
            if (batteryLvls[cube] != lvl) {
                batteryLvls[cube] = lvl;
                String<128> str;
                str << "bat:   " << Hex(lvl, 4) << "\n";
                vid[cube].bg0rom.text(vec(1,13), str);
            }
        }

        System::paint();
    }
}
