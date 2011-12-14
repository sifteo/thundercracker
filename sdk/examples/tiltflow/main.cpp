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
#include "MenuController.h"
#include "utils.h"

using namespace Sifteo;

static MenuController &menu = MenuController::Inst();

/*
static void onAccelChange(_SYSCubeID cid)
{
    _SYSAccelState state;
    _SYS_getAccel(cid, &state);

	static const int TILT_THRESHOLD = 20;

    menu.cubes[cid].ClearTiltInfo();

	if( state.x > TILT_THRESHOLD )
        menu.cubes[cid].AddTiltInfo( RIGHT );
	else if( state.x < -TILT_THRESHOLD )
        menu.cubes[cid].AddTiltInfo( LEFT );
	if( state.y > TILT_THRESHOLD )
        menu.cubes[cid].AddTiltInfo( DOWN );
	else if( state.y < -TILT_THRESHOLD )
        menu.cubes[cid].AddTiltInfo( UP);
}
*/

static void onTilt(_SYSCubeID cid)
{
    Cube::TiltState state = menu.cubes[cid].GetCube().getTiltState();

	if( state.x == _SYS_TILT_POSITIVE )
        menu.cubes[cid].Tilt( RIGHT );
	else if( state.x == _SYS_TILT_NEGATIVE )
        menu.cubes[cid].Tilt( LEFT );
	if( state.y == _SYS_TILT_POSITIVE )
        menu.cubes[cid].Tilt( DOWN );
	else if( state.y == _SYS_TILT_NEGATIVE )
        menu.cubes[cid].Tilt( UP);
}

static void onShake(_SYSCubeID cid)
{
    _SYS_ShakeState state;
    _SYS_getShake(cid, &state);
    menu.cubes[cid].Shake(state);
}

static void init()
{
    menu.Init();
}


void selectormain()
{
    init();

    //_SYS_vectors.cubeEvents.accelChange = onAccelChange;
    _SYS_vectors.cubeEvents.tilt = onTilt;
    _SYS_vectors.cubeEvents.shake = onShake;

    while (1) {
        menu.Update();
    }
}

void siftmain()
{
    return;
}

