/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _MENUCONTROLLER_H
#define _MENUCONTROLLER_H

#include <sifteo.h>
#include "cubewrapper.h"
#include "TiltFlowMenu.h"

using namespace Sifteo;

namespace SelectorMenu
{


void RunMenu();

//singleton class
class MenuController
{
public:    
    static MenuController &Inst();
	
    MenuController();

	static const int NUM_CUBES = 2;
    static const int NUM_MENU_ITEMS = 3;

	CubeWrapper cubes[NUM_CUBES]; 

	void Init();
	void Update();
	void Reset();
	

private:
    float m_fLastTime;
    TiltFlowMenu m_Menu;
};

} //namespace TiltFlowMenu

#endif
