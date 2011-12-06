/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * helper class for BG1
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
    memset( m_tileset, 0xff, 512 );
}

void BG1Helper::Flush()
{	
	unsigned int tileOffset = 0;

	//this part should be smarter (use ranges)
    //very unoptimal
	for (unsigned y = 0; y < BG1_ROWS; y++)
    {
		if( m_bitset[y] > 0 )
		{
			for( unsigned i = 0; i < BG1_COLS; i++ )
			{
                if( m_tileset[y][i] != 0xffff )
				{
					_SYS_vbuf_writei(&m_cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2 + tileOffset++,
                         m_tileset[y]+i,
                         0, 1);
				}
			}
		}
	}

    ASSERT( getBitSetCount() == tileOffset );

    //copy over our bitset
    _SYS_vbuf_write(&m_cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2,
                         m_bitset,
                         BG1_ROWS);

    //_SYS_vbuf_pokeb(&m_cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);

	Clear();
}

void BG1Helper::DrawAsset( const Vec2 &point, const Sifteo::AssetImage &asset, unsigned frame )
{
    ASSERT( frame < asset.frames );
    unsigned offset = asset.width * asset.height * frame;

    for (unsigned y = 0; y < asset.height; y++)
    {
        unsigned yOff = y + point.y;
        SetBitRange( yOff, point.x, asset.width );

        memcpy( m_tileset[yOff] + point.x, asset.tiles + offset, asset.width * 2 );

        offset += asset.width;
    }

	ASSERT( getBitSetCount() <= MAX_TILES );
}

//draw a partial asset.  Pass in the position, xy min points, and width/height
void BG1Helper::DrawPartialAsset( const Vec2 &point, const Vec2 &offset, const Vec2 &size, const Sifteo::AssetImage &asset, unsigned frame )
{
    unsigned tileOffset = asset.width * asset.height * frame + ( asset.width * offset.y ) + offset.x;

    for (int y = 0; y < size.y; y++)
    {
        unsigned yOff = y + point.y;
        SetBitRange( yOff, point.x, size.x );

        memcpy( m_tileset[yOff] + point.x, asset.tiles + tileOffset, size.x * 2 );

        tileOffset += asset.width;
    }

	ASSERT( getBitSetCount() <= MAX_TILES );
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
	//not quite sure why least significant bits appear on the right-most tiles
    setbits = setbits << xOffset;

    m_bitset[bitsetIndex] |= setbits;
}



//count how many bits set we have total
//only used for debug, so I don't care about optimizing it yet
unsigned int BG1Helper::getBitSetCount() const
{
    unsigned int count = 0;
	for (unsigned y = 0; y < BG1_ROWS; y++)
	{
		count += __builtin_popcount( m_bitset[y] );
	}

	return count;
}
