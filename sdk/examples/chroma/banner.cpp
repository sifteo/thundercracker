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
    m_bIsScoreMsg = false;
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


        if( m_bIsScoreMsg )
            bg1helper.DrawAsset( Vec2( iOffset, 7 ), BannerPoints, m_Msg[i] - '0' );
        else
            bg1helper.DrawAsset( Vec2( iOffset, 7 ), Font, m_Msg[i] - ' ' );
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


void Banner::SetMessage( const char *pMsg, bool bScoreMsg )
{
	ASSERT( strlen( pMsg ) < BANNER_WIDTH );

	if( strlen( pMsg ) < BANNER_WIDTH )
	{
		strcpy( m_Msg, pMsg );
        float msgTime = bScoreMsg ? SCORE_FADE_DELAY/2.0f : SCORE_FADE_DELAY;
        m_fEndTime = System::clock() + msgTime;
        m_tiles = 0;
        m_bIsScoreMsg = bScoreMsg;
	}
}


bool Banner::IsActive() const
{
	return m_Msg[0] != '\0';
}


void Banner::DrawScore( BG1Helper &bg1helper, const Vec2 &pos, int score )
{
    char buf[16];
    snprintf(buf, sizeof buf - 1, "%d", score );

    int iLen = strlen( buf );
    if( iLen == 0 )
        return;

    for( int i = 0; i < iLen; i++ )
    {
        int iOffset = pos.x + i;

        bg1helper.DrawAsset( Vec2( iOffset, pos.y ), BannerPoints, buf[i] - '0' );
    }
}
