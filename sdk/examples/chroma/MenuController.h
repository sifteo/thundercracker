/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _MENUCONTROLLER_H
#define _MENUCONTROLLER_H

//#include <sifteo.h>
#include "TFcubewrapper.h"
#include "TiltFlowMenu.h"
#include "config.h"

using namespace Sifteo;
using namespace SelectorMenu;

void RunMenu();

//singleton class
class MenuController
{
public:    
    static MenuController &Inst();
	
    MenuController();

    //static const int NUM_CUBES = 3;
    static const int NUM_MAIN_MENU_ITEMS = 4;

	CubeWrapper cubes[NUM_CUBES]; 

	void Init();
	bool Update();
	void Reset();
	
    void checkNeighbors();

private:
    float m_fLastTime;
    TiltFlowMenu m_Menu;
};

#endif
