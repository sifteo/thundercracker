#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static CubeID cube = 0;
static VideoBuffer vid;

static AssetSlot MainSlot = AssetSlot::allocate();

static Metadata M = Metadata()
    .title("TileBuffer test")
    .cubeRange(1);


static void testTileBuffer()
{
    /*
     * Relocated draw, from an asset group that's guaranteed not to
     * start at base address zero
     */
    
    vid.initMode(BG0_SPR_BG1);
    vid.attach(cube);

    {
        ScopedAssetLoader loader;
        loader.start( GameAssets, MainSlot, cube);

        while( !loader.isComplete() ) {
            System::paint();
        }
    }

    TileBuffer<16, 16> tileBuffer(cube);
    tileBuffer.erase( Transparent );
    tileBuffer.image( vec(0,0), PointFont );
    vid.bg1.maskedImage( tileBuffer, Transparent, 0);

    System::paint();
    System::finish();

    SCRIPT(LUA, util:assertScreenshot(cube, 'test0'));
}

void main()
{
    // Bootstrapping that would normally be done by the Launcher
    _SYS_enableCubes(cube.bit());
    ScopedAssetLoader loader;
    SCRIPT(LUA, System():setAssetLoaderBypass(true));
    MainSlot.erase();
    loader.start(BootAssets, MainSlot, cube);
    ASSERT(loader.isComplete());
    SCRIPT(LUA, System():setAssetLoaderBypass(false));

    SCRIPT(LUA,
        package.path = package.path .. ";../../lib/?.lua"
        require('siftulator')
        cube = Cube(0)
    );

    testTileBuffer();

    LOG("Success.\n");
}
