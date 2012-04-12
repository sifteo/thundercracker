#ifndef _SPRITENUMBER_H
#define _SPRITENUMBER_H

#include <sifteo.h>
#include "assets.gen.h"

//draw an up to 2-digit number using our number sprites

void DrawSpriteNum( VideoBuffer &vid, unsigned int number, Int2 pos )
{
    ASSERT( number < 100 );

    //show current puzzle
    if( number >= 10 )
    {
        vid.sprites[1].setImage(BannerPointsWhite, number / 10);
        vid.sprites[1].move(pos);
    }

    vid.sprites[0].setImage(BannerPointsWhite, number % 10);
    vid.sprites[0].move(pos.x + ( BannerPointsWhite.pixelWidth() ), pos.y);
}

#endif
