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
    TiltFlowItem( UI_Main_Menu_Blitz, UI_Main_Menu_Topbar ),
    TiltFlowItem( UI_Main_Menu_Puzzle, UI_Main_Menu_Topbar ),
    TiltFlowItem( UI_Main_Menu_Survival, UI_Main_Menu_Topbar ),
    TiltFlowItem( UI_Main_Menu_Settings, UI_Main_Menu_Topbar ),
};

MenuController &MenuController::Inst()
{
    static MenuController menu = MenuController();

    return menu;
}

MenuController::MenuController() : m_Menu( MAINMENUITEMS, NUM_MAIN_MENU_ITEMS, NUM_CUBES )
{
	//Reset();
}


void MenuController::Init()
{
    m_Menu.AssignViews();
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
}


CubeWrapper &MenuController::GetWrapper( unsigned int id )
{
    return Game::Inst().m_cubes[ id ];
}
