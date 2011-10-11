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
#include "game.h"
#include "utils.h"

using namespace Sifteo;

enum
{
	UP,
	LEFT, 
	DOWN,
	RIGHT
};

//stupid way to ensure seeding the randomizer before static inits
#ifdef _WIN32
class RandInit
{
public:
	RandInit()
	{
		srand((int)System::clock());
	}
};

static RandInit randInit;
#endif

static Game &game = Game::Inst();

static void onAccelChange(_SYSCubeID cid)
{
    _SYSAccelState state;
    _SYS_getAccel(cid, &state);

	static const int TILT_THRESHOLD = 50;

	if( game.getState() == Game::STATE_PLAYING )
	{
		//for now , just tilt cube 0
		if( state.x > TILT_THRESHOLD )
			game.cubes[cid].Tilt( RIGHT );
		else if( state.x < -TILT_THRESHOLD )
			game.cubes[cid].Tilt( LEFT );
		else if( state.y > TILT_THRESHOLD )
			game.cubes[cid].Tilt( DOWN );
		else if( state.y < -TILT_THRESHOLD )
			game.cubes[cid].Tilt( UP);
	}
	else if( game.getState() == Game::STATE_HELLODEMO )
	{
		game.cubes[cid].HelloTilt( state );
	}
}

static void init()
{
	game.Init();
}

void siftmain()
{
    init();

    /*vid.BG0_text(Vec2(2,1), Font, "Hello World!");
	vid.BG0_textf(Vec2(2,6), Font, "Time: %4u.%u", (int)t, (int)(t*10) % 10);
 */
    _SYS_vectors.accelChange = onAccelChange;

	for( int i = 0; i < Game::NUM_CUBES; i++ )
		onAccelChange(i);

    while (1) {
        game.Update();        
    }
}
