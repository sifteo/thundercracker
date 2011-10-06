/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "cubewrapper.h"
#include "assets.gen.h"

CubeWrapper::CubeWrapper( _SYSCubeID id ) : cube(id), vid(cube.vbuf), rom(cube.vbuf)
{
}


void CubeWrapper::Init( AssetGroup &assets )
{
	cube.enable();
    cube.loadAssets( assets );

    rom.init();
    rom.BG0_text(Vec2(1,1), "Loading...");
}

bool CubeWrapper::DrawProgress( AssetGroup &assets )
{
	rom.BG0_progressBar(Vec2(0,7), cube.assetProgress(GameAssets, vid.LCD_width), 2);
        
	return cube.assetDone(assets);
}

void CubeWrapper::Draw()
{
	vid.BG0_drawAsset(Vec2(1,10), Gem0, 2);
}


void CubeWrapper::vidInit()
{
	vid.init();
}