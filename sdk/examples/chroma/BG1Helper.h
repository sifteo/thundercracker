/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Helper class for bg1.  Paint into it as you would bg0, then call flush which translates these into proper bg1 calls
 * super unoptimized for now
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _BG1HELPER_H
#define _BG1HELPER_H

#include <sifteo.h>

using namespace Sifteo;

class BG1Helper
{
public:
	static const unsigned int BG1_ROWS = 16;
	static const unsigned int BG1_COLS = 16;

	BG1Helper( Cube &cube );

	void Clear();
	void Flush();
	void DrawAsset( const Vec2 &point, const Sifteo::AssetImage &asset, unsigned frame=0 );


private:
    //set a number of bits at xoffset of the current bitset
    void SetBitRange( unsigned int bitsetIndex, unsigned int xOffset, unsigned int number );

	//bitset of which tiles are active
	uint16_t m_bitset[BG1_ROWS];
	//actual contents of tiles
    ///rows, cols
	uint16_t m_tileset[BG1_ROWS][BG1_COLS];
	Cube &m_cube;
};

#endif
