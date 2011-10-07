/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <sifteo.h>
#include "assets.gen.h"
#include "cubewrapper.h"
#include "utils.h"

using namespace Sifteo;

enum
{
	UP,
	LEFT, 
	DOWN,
	RIGHT
};

/*static const int NUM_CUBES = 2;

static CubeWrapper cubes[NUM_CUBES] = 
{
	CubeWrapper((_SYSCubeID)0),
	CubeWrapper((_SYSCubeID)1),
};*/
static const int NUM_CUBES = 1;

//stupid way to ensure seeding the randomizer before static inits
class RandInit
{
public:
	RandInit()
	{
		srand((int)System::clock());
	}
};

static RandInit randInit;

static CubeWrapper cubes[NUM_CUBES] = 
{
	CubeWrapper((_SYSCubeID)0),
};


static void onAccelChange(_SYSCubeID cid)
{
    _SYSAccelState state;
    _SYS_getAccel(cid, &state);

	//for now , just tilt cube 0
	if( state.x > 105 )
		cubes[0].Tilt( RIGHT );
	else if( state.x < -105 )
		cubes[0].Tilt( LEFT );
	else if( state.y > 105 )
		cubes[0].Tilt( DOWN );
	else if( state.y < -105 )
		cubes[0].Tilt( UP);
}

static void init()
{
	for( int i = 0; i < NUM_CUBES; i++ )
		cubes[i].Init(GameAssets);

	bool done = false;

	PRINT( "getting ready to load" );

	while( !done )
	{
		done = true;
		for( int i = 0; i < NUM_CUBES; i++ )
		{
			if( !cubes[i].DrawProgress(GameAssets) )
				done = false;

			PRINT( "in load loop" );
		}
		System::paint();
	}
	PRINT( "done loading" );
	for( int i = 0; i < NUM_CUBES; i++ )
		cubes[i].vidInit();
}

void siftmain()
{
    init();

    /*vid.BG0_text(Vec2(2,1), Font, "Hello World!");
	vid.BG0_textf(Vec2(2,6), Font, "Time: %4u.%u", (int)t, (int)(t*10) % 10);
 */
    _SYS_vectors.accelChange = onAccelChange;

	for( int i = 0; i < NUM_CUBES; i++ )
		onAccelChange(i);

    while (1) {
        //float t = System::clock();
        
        for( int i = 0; i < NUM_CUBES; i++ )
			cubes[i].Draw();
            
        System::paint();
    }
}
