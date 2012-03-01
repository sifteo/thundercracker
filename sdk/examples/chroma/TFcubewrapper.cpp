/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "TFcubewrapper.h"
#include "MenuController.h"
#include "assets.gen.h"
#include "config.h"

static _SYSCubeID s_id = CUBE_ID_BASE;

namespace SelectorMenu
{

CubeWrapper::CubeWrapper() : m_cube(s_id++), m_vid(m_cube.vbuf), m_rom(m_cube.vbuf),
        m_bg1helper( m_cube )
{
}


void CubeWrapper::Init( AssetGroup &assets )
{
	m_cube.enable();
#if LOAD_ASSETS
    m_cube.loadAssets( assets );
#endif

    m_rom.init();
#if LOAD_ASSETS
    m_rom.BG0_text(Vec2(1,1), "Loading...");
#endif
}


void CubeWrapper::Reset()
{
}

void CubeWrapper::Draw()
{
}


void CubeWrapper::Update(float t, float dt)
{
}


void CubeWrapper::vidInit()
{
	m_vid.init();
}


void CubeWrapper::Tilt( int dir )
{
}


void CubeWrapper::Shake( bool bShaking )
{
}

} //namespace SelectorMenu
