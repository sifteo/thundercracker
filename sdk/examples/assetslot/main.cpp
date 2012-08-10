/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include <sifteo/menu.h>
#include "assets.gen.h"
#include "loader.h"
using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(BootstrapGroup);

static AssetSlot AnimationSlot = AssetSlot::allocate();

static const unsigned numCubes = 3;
static const CubeSet allCubes(0, numCubes);
static VideoBuffer vid[numCubes];
static Metadata M = Metadata()
    .title("Asset Slot Example")
    .package("com.sifteo.sdk.assetslot", "0.5")
    .icon(Icon)
    .cubeRange(numCubes);

static MyLoader loader(allCubes, MainSlot, vid);


void animation(const AssetImage &image)
{
    loader.load(image.assetGroup(), AnimationSlot);

    for (CubeID cube : allCubes) {
        vid[cube].initMode(BG0);
        vid[cube].attach(cube);
    }

    while (1) {
        unsigned frame = SystemTime::now().cycleFrame(2.0, image.numFrames());

        for (CubeID cube : allCubes) {
            vid[cube].bg0.image(vec(0,0), image, frame);
            if (cube.isTouching())
                return;
        }

        System::paint();
    }
}


void main()
{
    /*
     * Main Menu
     */

    while (1) {
        loader.load(MenuGroup, MainSlot);

        static MenuItem items[] = { {&MenuIconSpinny}, {&MenuIconBall}, {0} };
        static MenuAssets assets = { &MenuBg, &MenuFooter, &MenuLabelEmpty };

        Menu m(vid[0], &assets, items);
        MenuEvent e;

        for (CubeID cube : allCubes) {
            vid[cube].initMode(BG0);
            vid[cube].attach(cube);
            vid[cube].bg0.erase(MenuStripe);
        }

        while (m.pollEvent(&e))
            m.performDefault();

        switch (e.item) {
            case 0: animation(Spinny); break;
            case 1: animation(Ball); break;
        }

    }
}
