#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static const unsigned gNumCubes = 1;
static VideoBuffer vid;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(BootAssets);

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
    CubeID cube = 0;
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
    SCRIPT(LUA,
        package.path = package.path .. ";../../lib/?.lua"
        require('siftulator')
        cube = Cube(0)
    );

    testTileBuffer();
}
