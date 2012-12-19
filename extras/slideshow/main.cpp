#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static const int NUM_CUBES = 6;

static Metadata M = Metadata()
    .title("Slideshow")
    .package("com.sifteo.extras.slideshow", "0.2")
    .cubeRange(NUM_CUBES);

AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(BootstrapAssets);


static VideoBuffer vids[ NUM_CUBES ];

/*
AssetImage *CUBE1 = &Cube1[0];
AssetImage *CUBE2 = Cube2;
AssetImage *CUBE3 = Cube3;
AssetImage *CUBE4 = Cube4;
AssetImage *CUBE5 = Cube5;
AssetImage *CUBE6 = Cube6;
*/
static const AssetImage *IMAGELISTS[NUM_CUBES] =
{
    &Cube1[0],
    &Cube2[0],
    &Cube3[0],
    &Cube4[0],
    &Cube5[0],
    &Cube6[0],
};


static int NUM_IMAGES[NUM_CUBES] =
{
    sizeof( Cube1 ) / sizeof( AssetImage ),
    sizeof( Cube2 ) / sizeof( AssetImage ),
    sizeof( Cube3 ) / sizeof( AssetImage ),
    sizeof( Cube4 ) / sizeof( AssetImage ),
    sizeof( Cube5 ) / sizeof( AssetImage ),
    sizeof( Cube6 ) / sizeof( AssetImage ),
};


static int curImage[NUM_CUBES] =
{
    0,
    0,
    0,
    0,
    0,
    0,
};


static void onTouch(void *context, unsigned int cid)
{
    //LOG( "touching!\n" );

    if( CubeID(cid).isTouching() )
    {
        curImage[cid]++;

        //if( curImage[cid] < NUM_IMAGES[cid] )
        curImage[cid] = (curImage[cid] % NUM_IMAGES[cid]);
        {
            LOG( "number of images for this cube = %d\n", NUM_IMAGES[cid] );
            LOG( "showing cube %d image %d\n", cid, curImage[cid] );
            vids[cid].bg0.image( vec(0,0), IMAGELISTS[cid][curImage[cid]] );
        }
    }
}


void main()
{
    Events::cubeTouch.set( onTouch );

    for( int i = 0; i < NUM_CUBES; i++ )
    {
        vids[i].attach( i );
        vids[i].initMode( BG0 );
        //show the initial image
        vids[i].bg0.image( vec(0,0), IMAGELISTS[i][0] );
    }    

    while( 1 )
    {
        System::paint();
    }
}
