/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Queues up draws for all chromits, then draws them
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"
#include "ChromitDrawer.h"
#include "game.h"

ChromitDrawer::ChromitDrawer()
{
    Reset();
}


void ChromitDrawer::Reset()
{
    memset8( (uint8_t *)m_allInfo, 0, sizeof( m_allInfo ) );
}


//queue up a chromit to draw in this slot
void ChromitDrawer::queue( unsigned int cubeIndex, UByte2 pos, const AssetImage *pImg, uint8_t frame )
{
    SlotInfo &slot = m_allInfo[ cubeIndex ][ pos.x ][ pos.y ];

    if( slot.pImage != pImg || slot.frame != frame )
    {
        slot.pImage = pImg;
        slot.frame = frame;
        slot.bDifferent = true;
    }
}


void ChromitDrawer::drawAll()
{
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        VideoBuffer &vid = Game::Inst().m_cubes[i].GetVid();

        for( int row = 0; row < CubeWrapper::NUM_ROWS; row++ )
        {
            for( int col = 0; col < CubeWrapper::NUM_COLS; col++ )
            {
                SlotInfo &slot = m_allInfo[ i ][ row ][ col ];

                if( slot.bDifferent )
                {
                    Int2 vec = { col * 4, row * 4 };

                    vid.bg0.image( vec, *slot.pImage, slot.frame );
                    slot.bDifferent = false;
                }
            }
        }

    }
}
