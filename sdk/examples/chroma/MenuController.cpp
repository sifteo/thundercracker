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
    Cube::TiltState state = s_menu.cubes[cid].GetCube().getTiltState();

    if( state.x == _SYS_TILT_POSITIVE )
        s_menu.cubes[cid].Tilt( RIGHT );
    else if( state.x == _SYS_TILT_NEGATIVE )
        s_menu.cubes[cid].Tilt( LEFT );
    if( state.y == _SYS_TILT_POSITIVE )
        s_menu.cubes[cid].Tilt( DOWN );
    else if( state.y == _SYS_TILT_NEGATIVE )
        s_menu.cubes[cid].Tilt( UP);
}

static void onShake(_SYSCubeID cid)
{
    _SYS_ShakeState state;
    _SYS_getShake(cid, &state);
    s_menu.cubes[cid].Shake(state);
}


static void onNeighborAdd(_SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1)
{
    s_menu.checkNeighbors();
}


void RunMenu()
{
    s_menu.Init();

    _SYS_vectors.cubeEvents.tilt = onTilt;
    _SYS_vectors.cubeEvents.shake = onShake;
    _SYS_vectors.neighborEvents.add = onNeighborAdd;

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

    //CES hackery, fake power on
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        VidMode_BG0_ROM rom( cubes[i].GetCube().vbuf );
        rom.clear();
        rom.BG0_text(Vec2(1,1), "Chroma");
        rom.BG0_textf(Vec2(14,14), "%d", i + 1);
    }

    while( true )
    {
        bool satisfied = true;
        System::paint();

        for( int i = 0; i < NUM_CUBES; i++ )
        {
            if( i > 0 && cubes[i].GetCube().physicalNeighborAt(LEFT) != i - 1 )
            {
                satisfied = false;
                break;
            }

            if( i < NUM_CUBES - 1 && cubes[i].GetCube().physicalNeighborAt(RIGHT) != i + 1 )
            {
                satisfied = false;
                break;
            }
        }

        if( satisfied )
            break;
    }

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


} //namespace SelectorMenu
