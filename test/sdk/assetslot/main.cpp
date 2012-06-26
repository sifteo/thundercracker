#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot Slot1 = AssetSlot::allocate();
static AssetSlot Slot2 = AssetSlot::allocate();
static AssetSlot Slot3 = AssetSlot::allocate();
static AssetSlot Slot4 = AssetSlot::allocate();

static CubeSet cubes(0, 3);
static Metadata M = Metadata()
    .title("AssetSlot test")
    .cubeRange(3);

static VideoBuffer vid[CUBE_ALLOCATION];


bool load(AssetGroup &group, AssetSlot &slot, bool bypass)
{
    ScopedAssetLoader loader;
    bool result;

    if (bypass)
        SCRIPT(LUA, System():setAssetLoaderBypass(true));

    result = loader.start(group, slot, cubes);
    loader.finish();

    SCRIPT(LUA, System():setAssetLoaderBypass(false));

    return result;
}

void testGroupLoading()
{
    /*
     * Run through a set of loading operations once, starting with empty slots.
     */

    // Bind all four slots, using our own volume ID
    _SYS_asset_bindSlots(_SYS_fs_runningVolume(), 4);

    // All slots should start out empty, for the purposes of testing
    ASSERT(Slot1.tilesFree() == 4096);
    ASSERT(Slot2.tilesFree() == 4096);
    ASSERT(Slot3.tilesFree() == 4096);
    ASSERT(Slot4.tilesFree() == 4096);

    // Slot 1, load all of the numbered groups.
    // Do this twice. The second time will be a no-op.
    
    for (unsigned iter = 0; iter < 2; iter++) {
        unsigned addr = 0;
        unsigned base[CUBE_ALLOCATION];

        STATIC_ASSERT(arraysize(NumberList) == 25);
        for (unsigned n = 0; n < arraysize(NumberList); n++) {
            AssetGroup &group = NumberList[n].assetGroup();

            if (iter == 0) {
                // Verify initial sizes of slots
                ASSERT(Slot1.tilesFree() == 4096 - addr);
                ASSERT(Slot2.tilesFree() == 4096);
                ASSERT(Slot3.tilesFree() == 4096);
                ASSERT(Slot4.tilesFree() == 4096);
            }

            // The 25th group is expected to fail. All others should be loaded.
            ASSERT(group.isInstalled(cubes) == (iter != 0 && n < 24));
    
            // Test with and without asset loader bypass
            ASSERT(load(group, Slot1, (iter + n) & 1) == n < 24);
            ASSERT(group.isInstalled(cubes) == n < 24);

            if (group.isInstalled(cubes)) {

                // Verify load addresses
                for (CubeID cube : cubes) {
                    if (n) {
                        ASSERT(group.baseAddress(cube) == base[cube] + addr);
                    } else {
                        ASSERT(addr == 0);
                        base[cube] = group.baseAddress(cube);
                    }
                }

                // Verify final size of slot
                addr += (group.numTiles() + 15) & ~15;
                if (iter == 0)
                    ASSERT(Slot1.tilesFree() == 4096 - addr);
            }
        }
    }

    // Make sure we can use all of the loaded asset groups

    for (unsigned n = 0; n < 24; n++) {
        for (CubeID cube : cubes) {
            vid[cube].initMode(BG0);
            vid[cube].attach(cube);
            vid[cube].bg0.image(vec(0,0), NumberList[n]);
        }

        System::paint();
        System::finish();

        for (CubeID cube : cubes)
            SCRIPT_FMT(LUA, "util:assertScreenshot(Cube(%d), 'number-%d')", int(cube), n+1);
    }
}


void main()
{
    _SYS_enableCubes(cubes);

    SCRIPT(LUA,
        package.path = package.path .. ";../../lib/?.lua"
        require('siftulator')
    );

    testGroupLoading();

    LOG("Success.\n");
}