/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "MenuController.h"
#include "utils.h"
#include "assets.gen.h"


TiltFlowItem MENUITEMS[ MenuController::NUM_MENU_ITEMS ] =
{
    TiltFlowItem( IconFire, TextFire ),
    TiltFlowItem( IconEarth, TextEarth ),
    TiltFlowItem( IconWater, TextWater ),
};

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

    m_Menu.AssignViews();

	bool done = false;

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
	for( int i = 0; i < NUM_CUBES; i++ )
		cubes[i].vidInit();

    m_fLastTime = System::clock();
}


void MenuController::Update()
{
    float t = System::clock();
    float dt = t - m_fLastTime;
    m_fLastTime = t;

    m_Menu.Tick(dt);
            
    System::paint();
}


void MenuController::Reset()
{
	for( int i = 0; i < NUM_CUBES; i++ )
	{
		cubes[i].Reset();
	}
}
