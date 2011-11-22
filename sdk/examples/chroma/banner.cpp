/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * message banners that show up to display score or game info
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "banner.h"
#include "string.h"
#include "assets.gen.h"

Banner::Banner()
{
	m_Msg[0] = '\0';
	m_fEndTime = -1.0f;
    m_tiles = 0;
}


void Banner::Draw( BG1Helper &bg1helper )
{
	int iLen = strlen( m_Msg );
	if( iLen == 0 )
		return;

	ASSERT( iLen <= MAX_BANNER_LENGTH );

    bg1helper.DrawAsset( Vec2( 0, 6 ), BannerImg );

    int iStartXTile = ( BANNER_WIDTH - iLen ) / 2;

    for( int i = 0; i < iLen; i++ )
    {
        int iOffset = iStartXTile + i;

        bg1helper.DrawAsset( Vec2( iOffset, 7 ), Font, m_Msg[i] - ' ' );
    }

    /*
	_SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
	// Allocate tiles for the banner
    _SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, 0xFFFF, BANNER_ROWS );

	//clear banner
	for( unsigned int i = 0; i < BANNER_WIDTH * BANNER_ROWS; i++ )
	{
		_SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2 + i,
                         Font.tiles,
                         0, 1);
	}
	//draw banner
	int iStartXTile = ( BANNER_WIDTH - iLen ) / 2;

	for( int i = 0; i < iLen; i++ )
	{
		int iOffset = iStartXTile + i;

		//double tall
		for( int j = 0; j < 2; j++ )
		{
			_SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2 + iOffset + ( BANNER_WIDTH * ( j + 1 ) ),
							 Font.tiles + ( ( m_Msg[i] - ' ' ) * 2 ) + j,
							 0, 1);
		}
	}

	_SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_y), -48);
*/
}


void Banner::Update(float t, Cube &cube)
{
	int iLen = strlen( m_Msg );
	if( iLen > 0 )
	{
		if( t > m_fEndTime )
		{
			m_Msg[0] = '\0';
			m_fEndTime = -1.0f;
			//clear out
			_SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, 0x0000, BANNER_ROWS );
		}
	}
}


void Banner::SetMessage( const char *pMsg, float duration )
{
	ASSERT( strlen( pMsg ) < BANNER_WIDTH );

	if( strlen( pMsg ) < BANNER_WIDTH )
	{
		strcpy( m_Msg, pMsg );
		m_fEndTime = System::clock() + duration;
        m_tiles = 0;
	}
}


bool Banner::IsActive() const
{
	return m_Msg[0] != '\0';
}
