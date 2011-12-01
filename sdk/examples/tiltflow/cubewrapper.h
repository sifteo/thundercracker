/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBEWRAPPER_H
#define _CUBEWRAPPER_H

#include <sifteo.h>
#include "BG1Helper.h"

using namespace Sifteo;

//wrapper for a cube.  Contains the cube instance and video buffers, along with associated game information
class CubeWrapper
{
public:
	CubeWrapper();

	void Init( AssetGroup &assets );
	void Reset();
	//draw loading progress.  return true if done
	bool DrawProgress( AssetGroup &assets );
	void Draw();
    void Update(float t, float dt);
	void vidInit();
	void Tilt( int dir );
	void Shake( bool bShaking );

	Cube &GetCube() { return m_cube; }

private:
	Cube m_cube;
	VidMode_BG0 m_vid;
	VidMode_BG0_ROM m_rom;
	BG1Helper m_bg1helper;
};

#endif
