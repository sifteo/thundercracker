#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot Slot0(0);
static AssetSlot Slot1(1);
static AssetSlot Slot2(2);
static AssetSlot Slot3(3);

static CubeSet cubes(0, 3);
static Metadata M = Metadata()
    .title("AssetSlot test")
    .cubeRange(3);

static VideoBuffer vid[CUBE_ALLOCATION];

unsigned roundTiles(unsigned t)
{
    return (t + 15) & ~15;
}

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
     * Verifies the amount of free space at every step, and verifies base addresses.
     */

    LOG("================= Testing multi-group scenario\n");

    // Bind all four slots, using our own volume ID
    _SYS_asset_bindSlots(_SYS_fs_runningVolume(), 4);

    // All slots should start out empty, for the purposes of testing
    ASSERT(Slot0.tilesFree() == 4096);
    ASSERT(Slot1.tilesFree() == 4096);
    ASSERT(Slot2.tilesFree() == 4096);
    ASSERT(Slot3.tilesFree() == 4096);

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
                ASSERT(Slot0.tilesFree() == 4096 - addr);
                ASSERT(Slot1.tilesFree() == 4096);
                ASSERT(Slot2.tilesFree() == 4096);
                ASSERT(Slot3.tilesFree() == 4096);
            }

            // The 25th group is expected to fail. All others should be loaded.
            ASSERT(group.isInstalled(cubes) == (iter != 0 && n < 24));
    
            // Test with and without asset loader bypass
            ASSERT(load(group, Slot0, (iter + n) & 1) == n < 24);
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
                addr += roundTiles(group.numTiles());
                if (iter == 0)
                    ASSERT(Slot0.tilesFree() == 4096 - addr);
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


void testMultipleSlots()
{
    /*
     * Test some basic multiple-slot scenarios.
     *
     * We leave the existing groups in Slot 1, while loading
     * some large animations into the other slots.
     */

    LOG("================= Testing multi-slot scenario\n");

    // Bind all four slots, using our own volume ID
    _SYS_asset_bindSlots(_SYS_fs_runningVolume(), 4);

    ASSERT(Slot0.tilesFree() != 4096);
    ASSERT(Slot1.tilesFree() == 4096);
    ASSERT(Slot2.tilesFree() == 4096);
    ASSERT(Slot3.tilesFree() == 4096);

    ASSERT(load(Ball1Group, Slot1, true) == true);
    ASSERT(load(Ball2Group, Slot2, false) == true);
    ASSERT(load(Ball3Group, Slot3, true) == true);

    ASSERT(Slot1.tilesFree() == 4096 - roundTiles(Ball1Group.numTiles()));
    ASSERT(Slot2.tilesFree() == 4096 - roundTiles(Ball2Group.numTiles()));
    ASSERT(Slot3.tilesFree() == 4096 - roundTiles(Ball3Group.numTiles()));

    // Spot-check the resulting frames in arbitrary order

    const AssetImage *images[] = { &Ball1, &Ball2, &Ball3 };

    for (unsigned checks = 0; checks < 30; checks++) {
        const AssetImage *image = images[checks % arraysize(images)];
        unsigned frame = (checks * 3) % image->numFrames();

        for (CubeID cube : cubes) {
            vid[cube].initMode(BG0);
            vid[cube].attach(cube);
            vid[cube].bg0.image(vec(0,0), *image, frame);
        }

        System::paint();
        System::finish();

        for (CubeID cube : cubes)
            SCRIPT_FMT(LUA, "util:assertScreenshot(Cube(%d), 'img-%P-frame-%d', 20000)",
                int(cube), image, frame);
    }
}


void testFull()
{
    /*
     * For asset slots that are already partially full, try to completely fill them.
     */

    LOG("================= Testing completely full asset slots\n");

    // Bind all four slots, using our own volume ID
    _SYS_asset_bindSlots(_SYS_fs_runningVolume(), 4);

    AssetSlot slots[] = { Slot1, Slot2, Slot3 };

    for (unsigned s = 0; s < arraysize(slots); s++) {
        AssetSlot slot = slots[s];
        unsigned free = slot.tilesFree();

        ASSERT(free < 4096);

        for (unsigned n = 0; n < arraysize(NumberList); n++) {
            AssetGroup &group = NumberList[n].assetGroup();
            unsigned tiles = roundTiles(group.numTiles());
            ASSERT(free == slot.tilesFree());

            if (tiles > free) {
                ASSERT(load(group, slot, false) == false);
                break;
            } else {
                ASSERT(load(group, slot, false) == true);
                free -= tiles;
            }

            // No matter what, this group is still available in Slot0 :)
            ASSERT(group.isInstalled(cubes) == true);
        }
    }
}


void testCancel()
{
    /*
     * Test the indeterminate state slots are in during loading.
     */

    AssetGroup &g1 = NumberList[0].assetGroup();
    AssetGroup &g2 = NumberList[1].assetGroup();

    LOG("================= Testing cancellation\n");

    // Bind all four slots, using our own volume ID
    _SYS_asset_bindSlots(_SYS_fs_runningVolume(), 4);

    // The previous test should have left the first slot full. Erase it.
    ASSERT(Slot0.tilesFree() != 4096);
    ASSERT(Slot1.tilesFree() != 4096);
    ASSERT(Slot2.tilesFree() != 4096);
    ASSERT(Slot3.tilesFree() != 4096);
    Slot0.erase();
    Slot1.erase();
    Slot2.erase();
    Slot3.erase();
    ASSERT(Slot0.tilesFree() == 4096);
    ASSERT(Slot1.tilesFree() == 4096);
    ASSERT(Slot2.tilesFree() == 4096);
    ASSERT(Slot3.tilesFree() == 4096);
    
    // Load one small group
    ASSERT(g1.isInstalled(cubes) == false);
    ASSERT(load(g1, Slot0, false) == true);
    ASSERT(g1.isInstalled(cubes) == true);
    ASSERT(Slot0.tilesFree() < 4096);
    ASSERT(Slot0.tilesFree() > 3000);

    // Cancel the second one
    {
        ScopedAssetLoader loader;
        ASSERT(loader.start(g2, Slot0, cubes) == true);
        loader.cancel();
    }

    // For reading purposes, the first group should still be there, but
    // the second shouldn't be. (In-progress slots discard their most recent
    // group)
    ASSERT(g1.isInstalled(cubes) == true);
    ASSERT(g2.isInstalled(cubes) == false);

    // Nothing can be loaded here now.
    ASSERT(Slot0.tilesFree() == 0);
    {
        ScopedAssetLoader loader;
        ASSERT(loader.start(g2, Slot0, cubes) == false);
    }

    // Clear it and try again
    Slot0.erase();

    ASSERT(g1.isInstalled(cubes) == false);
    ASSERT(g2.isInstalled(cubes) == false);

    ASSERT(load(g1, Slot0, false) == true);
    ASSERT(load(g2, Slot0, false) == true);

    ASSERT(g2.isInstalled(cubes) == true);
    ASSERT(g1.isInstalled(cubes) == true);
}


void testBinding()
{
    /*
     * Test slot allocation and re-binding to different volumes
     */

    LOG("================= Testing slot allocation and binding\n");

    // Locate our four stub volumes
    Array<Volume, 8> vols;
    Volume::list(Volume::T_GAME, vols);
    ASSERT(vols.count() == 4);

    /*
     * Make sure we can bind a single slot on each of these stubs,
     * without evicting anything.
     */

    for (unsigned i = 0; i < 4; ++i) {

        // Make sure no previous slots were evicted
        for (unsigned j = 0; j < i; ++j) {
            LOG("Checking slot for %08x\n", vols[j].sys);
            _SYS_asset_bindSlots(vols[j], 1);
            ASSERT(Slot0.tilesFree() < 4096);
            ASSERT(NumberList[0].assetGroup().isInstalled(cubes) == true);
            ASSERT(Ball1Group.isInstalled(cubes) == false);
        }

        // Bind and load
        LOG("Loading slot for %08x\n", vols[i].sys);
        ASSERT(vols[i].sys != _SYS_fs_runningVolume());
        _SYS_asset_bindSlots(vols[i], 1);

        // Check immediately afterwards too, without a rebind
        ASSERT(Slot0.tilesFree() == 4096);
        load(NumberList[0].assetGroup(), Slot0, true);
        ASSERT(Slot0.tilesFree() < 4096);
        ASSERT(NumberList[0].assetGroup().isInstalled(cubes) == true);

        // Other volumes' assets are not reachable
        ASSERT(Ball1Group.isInstalled(cubes) == false);
    }

    _SYS_asset_bindSlots(_SYS_fs_runningVolume(), 4);
    ASSERT(Slot0.tilesFree() < 4096);
    ASSERT(Slot1.tilesFree() < 4096);
    ASSERT(Slot2.tilesFree() < 4096);
    ASSERT(Slot3.tilesFree() < 4096);
    ASSERT(Ball1Group.isInstalled(cubes) == true);
}


void testEviction()
{
    /*
     * Test eviction of existing bindings in order to create new ones
     */

    LOG("================= Testing slot binding eviction\n");

    // Locate our four stub volumes
    Array<Volume, 8> vols;
    Volume::list(Volume::T_GAME, vols);
    ASSERT(vols.count() == 4);

    // Set the access rank for existing slots, according to the order in which we've bound them.
    // This should give us access ranks 4, 3, 2, 1, and 0 respectively.

    _SYS_asset_bindSlots(vols[0], 1);
    ASSERT(Slot0.tilesFree() < 4096);
    
    _SYS_asset_bindSlots(vols[1], 1);
    ASSERT(Slot0.tilesFree() < 4096);

    _SYS_asset_bindSlots(vols[2], 1);
    ASSERT(Slot0.tilesFree() < 4096);

    _SYS_asset_bindSlots(vols[3], 1);
    ASSERT(Slot0.tilesFree() < 4096);

    _SYS_asset_bindSlots(Volume::running(), 4);
    ASSERT(Slot0.tilesFree() < 4096);
    ASSERT(Slot1.tilesFree() < 4096);
    ASSERT(Slot2.tilesFree() < 4096);
    ASSERT(Slot3.tilesFree() < 4096);

    // Enlarge the binding for vols[2]. This should automatically evict the existing
    // binding for that volume, plus it will need to evict vols[0].

    _SYS_asset_bindSlots(vols[2], 2);
    ASSERT(Slot0.tilesFree() == 4096);
    ASSERT(Slot1.tilesFree() == 4096);

    // Check over all non-evicted volumes

    _SYS_asset_bindSlots(vols[1], 1);
    ASSERT(Slot0.tilesFree() < 4096);

    _SYS_asset_bindSlots(vols[3], 1);
    ASSERT(Slot0.tilesFree() < 4096);

    _SYS_asset_bindSlots(Volume::running(), 4);
    ASSERT(Slot0.tilesFree() < 4096);
    ASSERT(Slot1.tilesFree() < 4096);
    ASSERT(Slot2.tilesFree() < 4096);
    ASSERT(Slot3.tilesFree() < 4096);

    // Verify that vols[0] was evicted

    _SYS_asset_bindSlots(vols[0], 1);
    ASSERT(Slot0.tilesFree() == 4096);
}


void main()
{
    while (CubeSet::connected() != cubes)
        System::yield();

    SCRIPT(LUA,
        package.path = package.path .. ";../../lib/?.lua"
        require('siftulator')
    );

    testGroupLoading();
    testMultipleSlots();
    testFull();
    testBinding();
    testEviction();
    testCancel();

    LOG("Success.\n");
}