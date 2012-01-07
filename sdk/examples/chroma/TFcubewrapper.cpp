/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "TFcubewrapper.h"
#include "MenuController.h"
#include "assets.gen.h"

using namespace SelectorMenu;

static _SYSCubeID s_id = CUBE_ID_BASE;

CubeWrapper::CubeWrapper() : m_cube(s_id++), m_vid(m_cube.vbuf), m_rom(m_cube.vbuf),
        m_bg1helper( m_cube )
{
}


void CubeWrapper::Init( AssetGroup &assets )
{
	m_cube.enable();
    m_cube.loadAssets( assets );

    m_rom.init();
    m_rom.BG0_text(Vec2(1,1), "Loading...");
}


void CubeWrapper::Reset()
{
}

bool CubeWrapper::DrawProgress( AssetGroup &assets )
{
    m_rom.BG0_progressBar(Vec2(0,7), m_cube.assetProgress(GameAssets, m_vid.LCD_width), 2);

    return m_cube.assetDone(assets);
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

