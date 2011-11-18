/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * message banners that show up to display score or game info
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "BG1Helper.h"
#include "string.h"

BG1Helper::BG1Helper( Cube &cube ) : m_cube( cube )
{
    Clear();
}


void BG1Helper::Clear()
{
    memset( m_bitset, 0, 32 );
    memset( m_tileset, 0, 512 );
}

void BG1Helper::Flush()
{
	_SYS_vbuf_pokeb(&m_cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
	//copy over our bitset
	_SYS_vbuf_writei(&m_cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2,
                         m_bitset,
                         0, BG1_ROWS);

	unsigned int tileOffset = 0;

	//this part should be smarter (use ranges)
	for (unsigned y = 0; y < BG1_ROWS; y++)
    {
		if( m_bitset[y] > 0 )
		{
			for( unsigned i = 0; i < BG1_COLS; i++ )
			{
				if( m_tileset[y][i] > 0 )
				{
					_SYS_vbuf_writei(&m_cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2 + tileOffset++,
                         m_tileset[y]+i,
                         0, 1);
				}
			}
		}
	}

	Clear();
}

void BG1Helper::DrawAsset( const Vec2 &point, const Sifteo::AssetImage &asset, unsigned frame )
{
    unsigned offset = asset.width * asset.height * frame;

    for (unsigned y = 0; y < asset.height; y++)
    {
        unsigned yOff = y + point.y;
        SetBitRange( yOff, point.x, point.x + asset.width );

        memcpy( m_tileset[yOff] + point.x, asset.tiles + offset, asset.width * 2 );

        offset += asset.width;
    }
}


//set a number of bits at xoffset of the current bitset
void BG1Helper::SetBitRange( unsigned int bitsetIndex, unsigned int xOffset, unsigned int number )
{
    ASSERT( bitsetIndex < 16 );
	ASSERT( xOffset < 16 );
	ASSERT( number > 0 );
    ASSERT( xOffset + number <= 16 );
    uint16_t setbits = ( 1 << number ) - 1;
    //how many bits from the end is this range?
    setbits = setbits << ( 16 - number - xOffset );

    m_bitset[bitsetIndex] |= setbits;
}
