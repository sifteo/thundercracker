/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>

using namespace Sifteo;

//wrapper for a cube.  Contains the cube instance and video buffers, along with associated game information
class CubeWrapper
{
public:
	CubeWrapper( _SYSCubeID id );

	void Init( AssetGroup &assets );
	//draw loading progress.  return true if done
	bool DrawProgress( AssetGroup &assets );
	void Draw();
	void vidInit();
private:
	Cube cube;
	VidMode_BG0 vid;
	VidMode_BG0_ROM rom;
};