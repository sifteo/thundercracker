/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "MenuController.h"
#include "Game.h"
#include "Utils.h"
#include "assets.gen.h"


TiltFlowItem MAINMENUITEMS[ MenuController::NUM_MAIN_MENU_ITEMS ] =
{
    TiltFlowItem( UI_Main_Menu_Survival, UI_Main_Menu_Topbar ),
    TiltFlowItem( UI_Main_Menu_Blitz, UI_Main_Menu_Topbar ),
    TiltFlowItem( UI_Main_Menu_Puzzle, UI_Main_Menu_Topbar ),
    TiltFlowItem( UI_Main_Menu_Settings, UI_Main_Menu_Topbar ),
};


static MenuController *s_pMenu = NULL;


MenuController &MenuController::Inst()
{
    ASSERT( s_pMenu );
    return *s_pMenu;
}

MenuController::MenuController() : m_Menu( MAINMENUITEMS, NUM_MAIN_MENU_ITEMS, NUM_CUBES )
{
    s_pMenu = this;
	//Reset();
}


void MenuController::Init()
{
    m_Menu.AssignViews();
}


bool MenuController::Update( int &choice )
{
    float t = System::clock();
    float dt = t - m_fLastTime;
    m_fLastTime = t;

    bool result = m_Menu.Tick(dt);
    choice = m_Menu.GetKeyView()->GetItem();
            
    System::paint();

	return result;
}

void MenuController::Reset()
{
}


CubeWrapper *MenuController::GetWrapper( Cube *pCube )
{
    return Game::Inst().GetWrapper( pCube );
}


CubeWrapper *MenuController::GetWrapper( unsigned int index )
{
    return Game::Inst().GetWrapper( index );
}
