#ifndef THING_H
#define THING_H
#include <sifteo.h>
#include "rect.h"

using namespace Sifteo;

const int PIXELS_PER_GRID = 32;

class ThingBounds{
  public:
    static const int NUM_RECTS = 2;
    Rect rect[NUM_RECTS];

    ThingBounds(Rect rect0, Rect rect1){
        rect[0] = rect0;
        rect[1] = rect1;
    }
};

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
//     int right(){ return pos.x + pixelWidth() - 1; }
//     int bottom(){ return pos.y + pixelHeight() - 1; }

    virtual ThingBounds bounds(){
        return ThingBounds( Rect(pos, Vec2(pixelWidth(), pixelHeight())), EMPTY_RECT );
    }

    bool isTouching(Thing &otherThing){
        ThingBounds myBounds = bounds();
        ThingBounds otherBounds = otherThing.bounds();
        return myBounds.rect[0].isTouching(otherBounds.rect[0])
            || myBounds.rect[0].isTouching(otherBounds.rect[1])
            || myBounds.rect[1].isTouching(otherBounds.rect[0])
            || myBounds.rect[1].isTouching(otherBounds.rect[1]);
    }

    void collided(Thing *otherThing){
        vel = Vec2(0.0f, 0.0f);
        pos.x = int(pos.x / PIXELS_PER_GRID + 0.5f) * PIXELS_PER_GRID;
        pos.y = int(pos.y / PIXELS_PER_GRID + 0.5f) * PIXELS_PER_GRID;
    }

};


class Platform : public Thing {

  public:

    Platform(int id, Int2 pos) : Thing(id, pos) {} 

    virtual void think(_SYSCubeID cubeId){
        _SYSTiltState tilt = _SYS_getTilt(cubeId);
        const float TILT_ACCELERATION = 2.0;
        vel.x += (tilt.x - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
        vel.y += (tilt.y - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
    }
};


class LPlatform : public Platform {
  private :
    typedef Platform super; // Private prevents erroneous use by other classes.
    

  public:

    static const int ORIENTATION_TOP_LEFT = 0;
    static const int ORIENTATION_TOP_RIGHT = 1;
    static const int ORIENTATION_BOTTOM_RIGHT = 2;
    static const int ORIENTATION_BOTTOM_LEFT = 3;
    char orientation;       // where the hole is in the L

    LPlatform(int id, Int2 pos) : Platform(id, pos) {
        orientation = ORIENTATION_TOP_LEFT;
    }

    void setSpriteImage(VidMode_BG0_SPR_BG1 &vid, const PinnedAssetImage &asset){
        pImage = &asset;
    }

    virtual void draw(VidMode_BG0_SPR_BG1 vid){
        ASSERT(pImage->frames == 4);

        int frameNum = orientation;
        vid.setSpriteImage(id, *pImage, frameNum);
        vid.moveSprite(id, pos.toInt());
    }

    virtual ThingBounds bounds(){
        switch(orientation){
            case ORIENTATION_TOP_LEFT:
                return ThingBounds( 
                        // top right
                        Rect(pos.x + PIXELS_PER_GRID, pos.y,     PIXELS_PER_GRID, PIXELS_PER_GRID),
                        // bottom
                        Rect(pos.x, pos.y + PIXELS_PER_GRID, 2 * PIXELS_PER_GRID, PIXELS_PER_GRID) );
            case ORIENTATION_TOP_RIGHT:
                return ThingBounds( 
                        // top left
                        Rect(pos.x, pos.y, PIXELS_PER_GRID, PIXELS_PER_GRID),
                        // bottom
                        Rect(pos.x, pos.y + PIXELS_PER_GRID, 2 * PIXELS_PER_GRID, PIXELS_PER_GRID) );
            case ORIENTATION_BOTTOM_LEFT:
                return ThingBounds(
                        // top
                        Rect(pos.x, pos.y, 2 * PIXELS_PER_GRID, PIXELS_PER_GRID),
                        // bottom right
                        Rect(pos.x, pos.y + PIXELS_PER_GRID, PIXELS_PER_GRID, PIXELS_PER_GRID) );
            case ORIENTATION_BOTTOM_RIGHT:
            default:
                
                return ThingBounds(
                        // top
                        Rect(pos.x, pos.y, 2 * PIXELS_PER_GRID, PIXELS_PER_GRID),
                        // bottom left
                        Rect(pos.x, pos.y, PIXELS_PER_GRID, PIXELS_PER_GRID) );
        }
    }

};

#endif // THING_H

