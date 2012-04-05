#ifndef THING_H
#define THING_H
#include <sifteo.h>
#include "rect.h"

using namespace Sifteo;

const int PIXELS_PER_GRID = 32;

const int NUM_BOUNDS_RECTS = 2;
// class ThingBounds{
//   public:
//     static const int NUM_RECTS = 2;
//     Rect rect[NUM_RECTS];

//     ThingBounds(Rect rect0, Rect rect1){
//         rect[0] = rect0;
//         rect[1] = rect1;
//     }
// };

class Thing {
  public:

    int id;
    Float2 pos;
    Float2 vel;
    const PinnedAssetImage *pImage;

    Thing(int id, Int2 pos){
        this->id = id;
        this->pos = pos.toFloat();
        pImage = NULL;
    }

    void setSpriteImage(VidMode_BG0_SPR_BG1 &vid, const PinnedAssetImage &asset){
        vid.setSpriteImage(id, asset);
        pImage = &asset;
    }


    virtual void think(_SYSCubeID cubeId){
    }

    void act(float dt){
//         const int MAX_VEL = 200.0;
//         if (vel.len() > MAX_VEL) vel = vel.normalize() * MAX_VEL;
//         vel = vel * 0.95;       // friction

        pos = pos + (vel * dt);

        if (   pos.x < 0.0 || (pos.x + pixelWidth()) > VidMode::LCD_width
            || pos.y < 0.0 || (pos.y + pixelHeight()) > VidMode::LCD_height){
            collided(NULL);
        }
    }

    virtual void draw(VidMode_BG0_SPR_BG1 vid){
        vid.moveSprite(id, pos.toInt());
    }

    int pixelWidth(){ return pImage->pixelWidth(); }
    int pixelHeight(){ return pImage->pixelHeight(); }

    virtual void bounds(Rect result[NUM_BOUNDS_RECTS]){
        result[0] = Rect(pos, Vec2(pixelWidth(), pixelHeight()));
        result[1] = EMPTY_RECT;
    }

    bool isTouching(Thing &otherThing){
        Rect myBounds[NUM_BOUNDS_RECTS];
        Rect otherBounds[NUM_BOUNDS_RECTS];

        bounds(myBounds);
        otherThing.bounds(otherBounds);

        return myBounds[0].isTouching(otherBounds[0])
            || myBounds[0].isTouching(otherBounds[1])
            || myBounds[1].isTouching(otherBounds[0])
            || myBounds[1].isTouching(otherBounds[1]);
    }

    void collided(Thing *otherThing){
        vel = Vec2(0.0f, 0.0f);
        pos.x = int(pos.x / PIXELS_PER_GRID + 0.5f) * PIXELS_PER_GRID;
        pos.y = int(pos.y / PIXELS_PER_GRID + 0.5f) * PIXELS_PER_GRID;
    }

};


#endif // THING_H

