/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _MENUCONTROLLER_H
#define _MENUCONTROLLER_H

//#include <sifteo.h>
#include "TiltFlowMenu.h"
#include "cubewrapper.h"
#include "config.h"

using namespace Sifteo;

void RunMenu();

//singleton class
class MenuController
{
public:    
    static MenuController &Inst();
	
    MenuController();

    //static const int NUM_CUBES = 3;
    static const int NUM_MAIN_MENU_ITEMS = 4;

	void Init();
    bool Update( int &choice );
	void Reset();
	
    void checkNeighbors();
    CubeWrapper *GetWrapper( Cube *pCube );
    CubeWrapper *GetWrapper( unsigned int index );

private:
    float m_fLastTime;
    TiltFlowMenu m_Menu;
};

#endif
