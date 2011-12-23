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

    //bg1helper.DrawAsset( Vec2( 0, 6 ), BannerImg );
    bg1helper.DrawPartialAsset( Vec2( CENTER_PT - m_tiles, 6 ), Vec2( CENTER_PT - m_tiles, 0 ), Vec2( m_tiles * 2, BANNER_ROWS ), BannerImg );

    int iStartXTile = ( BANNER_WIDTH - iLen ) / 2;

    for( int i = 0; i < iLen; i++ )
    {
        int iOffset = iStartXTile + i;

        //bg1helper.DrawAsset( Vec2( iOffset, 7 ), Font, m_Msg[i] - ' ' );
        bg1helper.DrawAsset( Vec2( iOffset, 7 ), BannerPoints, m_Msg[i] - '0' );
    }
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
		}
        m_tiles++;

        if( m_tiles > CENTER_PT )
            m_tiles = CENTER_PT;
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
