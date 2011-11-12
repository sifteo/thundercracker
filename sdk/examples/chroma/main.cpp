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

static void onTilt(_SYSCubeID cid)
{
    Cube::TiltState state = game.cubes[cid].GetCube().getTiltState();

	if( state.x == _SYS_TILT_POSITIVE )
		game.cubes[cid].Tilt( RIGHT );
	else if( state.x == _SYS_TILT_NEGATIVE )
		game.cubes[cid].Tilt( LEFT );
	if( state.y == _SYS_TILT_POSITIVE )
		game.cubes[cid].Tilt( DOWN );
	else if( state.y == _SYS_TILT_NEGATIVE )
		game.cubes[cid].Tilt( UP);
}

static void onShake(_SYSCubeID cid)
{
    _SYS_ShakeState state;
    _SYS_getShake(cid, &state);
	game.cubes[cid].Shake(state);
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
    _SYS_vectors.cubeEvents.tilt = onTilt;
	_SYS_vectors.cubeEvents.shake = onShake;

    while (1) {
        game.Update();        
    }
}
