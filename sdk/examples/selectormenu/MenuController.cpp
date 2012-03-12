/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "MenuController.h"
#include "TFutils.h"
#include "assets.gen.h"

namespace SelectorMenu
{

TiltFlowItem MENUITEMS[ MenuController::NUM_MENU_ITEMS ] =
{
    TiltFlowItem( IconGameChroma, LabelChroma ),
    TiltFlowItem( IconGameWord, LabelWord ),
    TiltFlowItem( IconGameSandwich, LabelSandwich ),
    //TiltFlowItem( IconGamePeano, LabelPeano ),
    TiltFlowItem( IconGameMore, LabelMore ),
};

MenuController &s_menu = MenuController::Inst();

/*
static void onAccelChange(void *context, _SYSCubeID cid)
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

static void onTilt(void *context, _SYSCubeID cid)
{
	//TODO, make not reliant on id base
    Cube::TiltState state = s_menu.cubes[cid - CUBE_ID_BASE].GetCube().getTiltState();

    if( state.x == _SYS_TILT_POSITIVE )
        s_menu.cubes[cid - CUBE_ID_BASE].Tilt( RIGHT );
    else if( state.x == _SYS_TILT_NEGATIVE )
        s_menu.cubes[cid - CUBE_ID_BASE].Tilt( LEFT );
    if( state.y == _SYS_TILT_POSITIVE )
        s_menu.cubes[cid - CUBE_ID_BASE].Tilt( DOWN );
    else if( state.y == _SYS_TILT_NEGATIVE )
        s_menu.cubes[cid - CUBE_ID_BASE].Tilt( UP);
}

static void onShake(void *context, _SYSCubeID cid)
{
    _SYSShakeState state = _SYS_getShake(cid);
    s_menu.cubes[cid - CUBE_ID_BASE].Shake(state);
}


static void onTouch(void *context, _SYSCubeID cid)
{
    s_menu.press( cid - CUBE_ID_BASE );
}


static void onNeighborAdd(void *context,
    _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1)
{
    s_menu.checkNeighbors();
}


void RunMenu()
{
    s_menu.Init();

    _SYS_setVector(_SYS_CUBE_TILT, (void*) onTilt, NULL);
    _SYS_setVector(_SYS_CUBE_SHAKE, (void*) onShake, NULL);
	_SYS_setVector(_SYS_CUBE_TOUCH, (void*) onTouch, NULL);
    _SYS_setVector(_SYS_NEIGHBOR_ADD, (void*) onNeighborAdd, NULL);

    while (s_menu.Update()) {}
}


MenuController &MenuController::Inst()
{
    static MenuController menu = MenuController();

    return menu;
}

MenuController::MenuController() : m_Menu( MENUITEMS, NUM_MENU_ITEMS, NUM_CUBES )
{
	//Reset();
}


void MenuController::Init()
{
	for( int i = 0; i < NUM_CUBES; i++ )
        cubes[i].Init(MenuControllerAssets);

    bool done = false;

#if LOAD_ASSETS
    PRINT( "getting ready to load" );

	while( !done )
	{
		done = true;
		for( int i = 0; i < NUM_CUBES; i++ )
		{
            if( !cubes[i].DrawProgress(MenuControllerAssets) )
				done = false;

			PRINT( "in load loop" );
		}
		System::paint();
	}
	PRINT( "done loading" );
#endif
    m_Menu.AssignViews();
	for( int i = 0; i < NUM_CUBES; i++ )
		cubes[i].vidInit();

	m_Menu.showLogo();
    m_fLastTime = System::clock();
}


bool MenuController::Update()
{
    float t = System::clock();
    float dt = t - m_fLastTime;
    m_fLastTime = t;

    bool result = m_Menu.Tick(dt);
            
    System::paint();

	return result;
}

void MenuController::Reset()
{
	for( int i = 0; i < NUM_CUBES; i++ )
	{
		cubes[i].Reset();
	}
}

void MenuController::checkNeighbors()
{
    m_Menu.checkNeighbors();
}

void MenuController::press( int cube )
{
    m_Menu.press( cube );
}


} //namespace SelectorMenu


void siftmain()
{
    SelectorMenu::RunMenu();
}