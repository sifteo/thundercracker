/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"
#include "cubewrapper.h"
#include "game.h"
#include "utils.h"

using namespace Sifteo;

static Game &game = Game::Inst();

/*
static void onAccelChange(void *context, _SYSCubeID cid)
{
    _SYSAccelState state;
    _SYS_getAccel(cid, &state);

	static const int TILT_THRESHOLD = 20;

	game.m_cubes[cid].ClearTiltInfo();

	if( state.x > TILT_THRESHOLD )
		game.m_cubes[cid].AddTiltInfo( RIGHT );
	else if( state.x < -TILT_THRESHOLD )
		game.m_cubes[cid].AddTiltInfo( LEFT );
	if( state.y > TILT_THRESHOLD )
		game.m_cubes[cid].AddTiltInfo( DOWN );
	else if( state.y < -TILT_THRESHOLD )
		game.m_cubes[cid].AddTiltInfo( UP);
}
*/

static void onTilt(void *context, _SYSCubeID cid)
{
    Cube::TiltState state = game.m_cubes[cid - CUBE_ID_BASE].GetCube().getTiltState();

	if( state.x == _SYS_TILT_POSITIVE )
        game.m_cubes[cid - CUBE_ID_BASE].Tilt( RIGHT );
	else if( state.x == _SYS_TILT_NEGATIVE )
        game.m_cubes[cid - CUBE_ID_BASE].Tilt( LEFT );
	if( state.y == _SYS_TILT_POSITIVE )
        game.m_cubes[cid - CUBE_ID_BASE].Tilt( DOWN );
	else if( state.y == _SYS_TILT_NEGATIVE )
        game.m_cubes[cid - CUBE_ID_BASE].Tilt( UP);
}

static void onShake(void *context, _SYSCubeID cid)
{
    _SYSShakeState state = (_SYSShakeState) _SYS_getShake(cid);
    game.m_cubes[cid - CUBE_ID_BASE].Shake(state);
}

static void init()
{
	game.Init();
}

void main()
{
    init();

    //_SYS_setVector(_SYS_CUBE_ACCELCHANGE, (void*) onAccelChange, NULL);
    _SYS_setVector(_SYS_CUBE_TILT, (void*) onTilt, NULL);
    _SYS_setVector(_SYS_CUBE_SHAKE, (void*) onShake, NULL);

    while (1) {
        game.Update();        
    }
}


void assertWrapper( bool value )
{
    if( !value )
    {
        int *p = 0;
        //force crash
        *p = 23;
    }
}
