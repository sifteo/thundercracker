#ifndef PLATFORM_H
#define PLATFORM_H

#include "thing.h"

class Platform : public Thing {
  private :
    typedef Platform super; // Private prevents erroneous use by other classes.

  public:

    Platform(World &world, int id, Int2 pos) : Thing(world, id, pos) {} 

    virtual void think(_SYSCubeID cubeId){
        _SYSTiltState tilt = _SYS_getTilt(cubeId);
        const float TILT_ACCELERATION = 2.0;
        vel.x += (tilt.x - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
        vel.y += (tilt.y - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
    }

    virtual void act(float dt){
        if (pWorld->isTurtleMoving) return;
        super::act(dt);
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

    LPlatform(World &world, int id, Int2 pos) : Platform(world, id, pos) {
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

    virtual void bounds(Rect result[NUM_BOUNDS_RECTS]){
        switch(orientation){
            case ORIENTATION_TOP_LEFT:
                // top right
                result[0] =Rect(pos.x + PIXELS_PER_GRID, pos.y,     PIXELS_PER_GRID, PIXELS_PER_GRID);
                // bottom
                result[1] = Rect(pos.x, pos.y + PIXELS_PER_GRID, 2 * PIXELS_PER_GRID, PIXELS_PER_GRID);
                break;
            case ORIENTATION_TOP_RIGHT:
                // top left
                result[0] = Rect(pos.x, pos.y, PIXELS_PER_GRID, PIXELS_PER_GRID);
                // bottom
                result[1] = Rect(pos.x, pos.y + PIXELS_PER_GRID, 2 * PIXELS_PER_GRID, PIXELS_PER_GRID);
                break;
            case ORIENTATION_BOTTOM_LEFT:
                // top
                result[0] = Rect(pos.x, pos.y, 2 * PIXELS_PER_GRID, PIXELS_PER_GRID);
                // bottom right
                result[1] = Rect(pos.x, pos.y + PIXELS_PER_GRID, PIXELS_PER_GRID, PIXELS_PER_GRID);
                break;
            case ORIENTATION_BOTTOM_RIGHT:
            default:
                // top
                result[0] = Rect(pos.x, pos.y, 2 * PIXELS_PER_GRID, PIXELS_PER_GRID);
                // bottom left
                result[1] = Rect(pos.x, pos.y, PIXELS_PER_GRID, PIXELS_PER_GRID);
        }
    }

};

#endif  // PLATFORM_H
