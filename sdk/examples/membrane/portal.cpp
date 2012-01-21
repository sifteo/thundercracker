/* Copyright <c> 2011 Sifteo, Inc. All rights reserved. */

#include <stdlib.h>
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
    const unsigned maxFrame = PlayfieldE.frames - 1;
    
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

void Portal::draw(Cube &cube)
{
    static const struct {
        const AssetImage *asset;
        Vec2 pos;
    } info[NUM_SIDES] = {
        { &PlayfieldE, Vec2(2,  0)  },
        { &PlayfieldW, Vec2(0,  2)  },
        { &PlayfieldF, Vec2(2,  13) },
        { &PlayfieldA, Vec2(13, 2)  },
    };
    
    VidMode_BG0 vid(cube.vbuf);
    vid.BG0_drawAsset(info[side].pos, *info[side].asset, frame);
}

Vec2F Portal::getTarget() const
{
    static const Vec2F center[NUM_SIDES] = {
        {  64,    -20 },
        { -20,     64 },
        {  64,     128+20 },
        {  128+20, 64 },
    };
    
    return center[side];
}

Vec2F Portal::rotateTo(const Portal &dest, Vec2F coord)
{
    return coord.rotate((dest.side - side) * (M_PI/2));
}

float Portal::distanceFrom(Vec2F coord)
{
    switch (side) {    
    default:
    case SIDE_TOP:     return coord.y;
    case SIDE_LEFT:    return coord.x;
    case SIDE_BOTTOM:  return VidMode::LCD_height - coord.y;
    case SIDE_RIGHT:   return VidMode::LCD_width - coord.x;
    }
}
