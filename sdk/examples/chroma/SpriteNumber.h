#ifndef _SPRITENUMBER_H
#define _SPRITENUMBER_H

#include <sifteo.h>
#include "assets.gen.h"

//draw an up to 2-digit number using our number sprites

void DrawSpriteNum( VidMode_BG0_SPR_BG1 &vid, unsigned int number, Int2 pos )
{
    ASSERT( number < 100 );

    //show current puzzle
    if( number >= 10 )
    {
        vid.resizeSprite( 1, BannerPointsWhite.width * 8, BannerPointsWhite.height * 8 );
        vid.setSpriteImage(1, BannerPointsWhite, number / 10);
        vid.moveSprite(1, pos.x, pos.y);
    }

    vid.resizeSprite( 0, BannerPointsWhite.width * 8, BannerPointsWhite.height * 8 );
    vid.setSpriteImage(0, BannerPointsWhite, number % 10);
    vid.moveSprite(0, pos.x + ( BannerPointsWhite.width * 8 ), pos.y);
}

#endif
