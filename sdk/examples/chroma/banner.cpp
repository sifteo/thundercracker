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
	m_fStartTime = -1.0f;
}


void Banner::Draw( Cube &cube )
{
	int iLen = strlen( m_Msg );
	if( iLen == 0 )
		return;

	ASSERT( iLen <= MAX_BANNER_LENGTH );

	_SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
	// Allocate tiles for the banner
    _SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, 0xFFFF, BANNER_ROWS );

	//clear banner
	for( int i = 0; i < BANNER_WIDTH * BANNER_ROWS; i++ )
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
	/*_SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2,
                         Cover.tiles,
                         0, BANNER_WIDTH * BANNER_ROWS);*/
}


void Banner::Update(float t)
{
}