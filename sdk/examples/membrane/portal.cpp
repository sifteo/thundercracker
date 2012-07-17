/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "game.h"


Portal::Portal(int side)
    : state(S_CLOSED), side(side), frame(0)
    {}
    
void Portal::setOpen(bool open)
{
    if (open && state != S_OPEN)
        state = S_OPENING;
    if (!open && state != S_CLOSED)
        state = S_CLOSING;
}

void Portal::animate()
{
    const unsigned maxFrame = PlayfieldE.numFrames() - 1;
    
    switch (state) {
    
    case S_CLOSING:
        if (!frame || !--frame)
            state = S_CLOSED;
        break;
    
    case S_OPENING:
        if (frame >= maxFrame || ++frame >= maxFrame)
            state = S_OPEN;
        break;
        
    case S_CLOSED:
        frame = 0;
        break;
        
    case S_OPEN:
        if (frame == maxFrame)
            frame = maxFrame - 1;
        else
            frame = maxFrame;
        break;
    }
}

void Portal::draw(VideoBuffer &vid)
{
    static const struct {
        const AssetImage *asset;
        Byte2 pos;
    } info[NUM_SIDES] = {
        { &PlayfieldE, { 2,  0  }  },
        { &PlayfieldW, { 0,  2  }  },
        { &PlayfieldF, { 2,  13 } },
        { &PlayfieldA, { 13, 2  }  },
    };

    vid.bg0.image(info[side].pos, *info[side].asset, frame);
}

Float2 Portal::getTarget() const
{
    static const Float2 center[] = {
        {  64,    -20     },
        { -20,     64     },
        {  64,     128+20 },
        {  128+20, 64     },
    };
    
    return center[side];
}

Float2 Portal::rotateTo(const Portal &dest, const Float2 &coord)
{
    return coord.rotate((dest.side - side) * (M_PI/2));
}

float Portal::distanceFrom(Float2 coord)
{
    switch (side) {    
    default:
    case TOP:     return coord.y;
    case LEFT:    return coord.x;
    case BOTTOM:  return LCD_height - coord.y;
    case RIGHT:   return LCD_width - coord.x;
    }
}
