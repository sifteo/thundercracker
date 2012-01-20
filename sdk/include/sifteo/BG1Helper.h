/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Helper class for bg1.  Paint into it as you would bg0, then call flush which translates these into proper bg1 calls
 * super unoptimized for now
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _BG1HELPER_H
#define _BG1HELPER_H

#include <sifteo.h>

using namespace Sifteo;

/*
 * XXX: This is a kludge to save memory for the moment, by not inlining
 *      frequently used but complex functions. The long-term solution is
 *      to refactor code into system calls as necessary to avoid unnecessary
 *      inlining. (If there's one copy of this code, it should live in the firmware
 *      instead of in each game binary)
 */
#ifndef NEVER_INLINE
#define NEVER_INLINE    __attribute__ ((noinline))
#endif

class BG1Helper
{
public:
	static const unsigned int BG1_ROWS = 16;
	static const unsigned int BG1_COLS = 16;
    static const unsigned int MAX_TILES = 144;

    BG1Helper( Cube &cube ) : m_cube( cube )
    {
        Clear();
    }

    NEVER_INLINE void Clear()
    {
        _SYS_memset16( &m_bitset[0], 0, BG1_ROWS );
        _SYS_memset16( &m_tileset[0][0], 0xffff, BG1_ROWS * BG1_COLS);
    }

    NEVER_INLINE void Flush()
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

        _SYS_vbuf_pokeb(&m_cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);	

        //store off last bitset for comparison later
        _SYS_memcpy16( m_lastbitset, m_bitset, BG1_ROWS );

        Clear();
    }

    NEVER_INLINE void DrawAsset( const Vec2 &point, const Sifteo::AssetImage &asset, unsigned frame=0 )
    {
        ASSERT( frame < asset.frames );
        unsigned offset = asset.width * asset.height * frame;

        for (unsigned y = 0; y < asset.height; y++)
        {
            unsigned yOff = y + point.y;
            SetBitRange( yOff, point.x, asset.width );

            _SYS_memcpy16( m_tileset[yOff] + point.x, asset.tiles + offset, asset.width );

            offset += asset.width;
        }

        ASSERT( getBitSetCount() <= MAX_TILES );
    }


	//draw a partial asset.  Pass in the position, xy min points, and width/height
    NEVER_INLINE void DrawPartialAsset( const Vec2 &point, const Vec2 &offset, const Vec2 &size, const Sifteo::AssetImage &asset, unsigned frame=0 )
    {
        ASSERT( frame < asset.frames );
        unsigned tileOffset = asset.width * asset.height * frame + ( asset.width * offset.y ) + offset.x;

        for (int y = 0; y < size.y; y++)
        {
            unsigned yOff = y + point.y;
            SetBitRange( yOff, point.x, size.x );

            _SYS_memcpy16( m_tileset[yOff] + point.x, asset.tiles + tileOffset, size.x );

            tileOffset += asset.width;
        }

        ASSERT( getBitSetCount() <= MAX_TILES );
    }


    void DrawText(const Vec2 &point, const Sifteo::AssetImage &font, char c) {
        unsigned index = c - (int)' ';
        if (index < font.frames)
            DrawAsset(point, font, index);
    }


    NEVER_INLINE void DrawText( const Vec2 &point, const Sifteo::AssetImage &font, const char *str )
    {
        Vec2 p = point;
        char c;

        while ((c = *str)) {
            if (c == '\n') {
                p.x = point.x;
                p.y += font.height;
            } else {
                DrawText(p, font, c);
                p.x += font.width;
            }
            str++;
        }
    }

    NEVER_INLINE void DrawTextf( const Vec2 &point, const Sifteo::AssetImage &font, const char *fmt, ... )
    {
        char buf[128];
        va_list ap;

        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf - 1, fmt, ap);
        buf[sizeof buf - 1] = 0;
        DrawText(point, font, buf);
        va_end(ap);
    }

    inline bool NeedFinish()
    {
        for( unsigned int i = 0; i < BG1_ROWS; i++ )
        {
            if( m_bitset[i] != m_lastbitset[i] )
            {
                return true;
            }
        }

        return false;
    }

private:
    //set a number of bits at xoffset of the current bitset
    NEVER_INLINE void SetBitRange( unsigned int bitsetIndex, unsigned int xOffset, unsigned int number )
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
    NEVER_INLINE unsigned int getBitSetCount() const
    {
        unsigned int count = 0;
        for (unsigned y = 0; y < BG1_ROWS; y++)
        {
            count += __builtin_popcount( m_bitset[y] );
        }

        return count;
    }

	//bitset of which tiles are active
	uint16_t m_bitset[BG1_ROWS];
	//last flushed bitset, used to tell whether we've made changes that will require a finish() to be called before the next frame
    uint16_t m_lastbitset[BG1_ROWS];

	//actual contents of tiles
    ///rows, cols
	uint16_t m_tileset[BG1_ROWS][BG1_COLS];
	Cube &m_cube;
};

#endif
