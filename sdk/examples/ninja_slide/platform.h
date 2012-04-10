#ifndef PLATFORM_H
#define PLATFORM_H

#include "thing.h"

class Platform : public Thing {
  private:
    typedef Thing super;
    
  public:

    Platform(World &world, int id, Int2 pos) : Thing(world, id, pos) {} 

    virtual void think(_SYSCubeID cubeId){
        _SYSTiltState tilt = _SYS_getTilt(cubeId);
        const float TILT_ACCELERATION = 2.0;
        vel.x += (tilt.x - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
        vel.y += (tilt.y - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
        if (!isMoving && vel.len2() > 0.1){
            isMoving = true;
            pWorld->numMovingPlatforms++;
        }
    }

    virtual void onCollision(Thing *other){
        if (isMoving){
            isMoving = false;
            pWorld->numMovingPlatforms--;
        }
        super::onCollision(other);
    }
};

class LPlatform : public Platform {
  private:
    typedef Platform super; // Private prevents erroneous use by other classes.
    

  public:

    static const int ORIENTATION_TOP_LEFT = 0;
    static const int ORIENTATION_TOP_RIGHT = 1;
    static const int ORIENTATION_BOTTOM_RIGHT = 2;
    static const int ORIENTATION_BOTTOM_LEFT = 3;
    char orientation;       // where the hole is in the L
    
    virtual bool occupiesCell(CellNum dest){
        Float2 destPoint = cellNumToPoint(dest);
        Rect theBounds[NUM_BOUNDS_RECTS];
        bool result = this->contains(destPoint);
        return result;
    }

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
                result[0] =Rect(pos.x + World::PIXELS_PER_CELL, pos.y,     World::PIXELS_PER_CELL, World::PIXELS_PER_CELL);
                // bottom
                result[1] = Rect(pos.x, pos.y + World::PIXELS_PER_CELL, 2 * World::PIXELS_PER_CELL, World::PIXELS_PER_CELL);
                break;
            case ORIENTATION_TOP_RIGHT:
                // top left
                result[0] = Rect(pos.x, pos.y, World::PIXELS_PER_CELL, World::PIXELS_PER_CELL);
                // bottom
                result[1] = Rect(pos.x, pos.y + World::PIXELS_PER_CELL, 2 * World::PIXELS_PER_CELL, World::PIXELS_PER_CELL);
                break;
            case ORIENTATION_BOTTOM_LEFT:
                // top
                result[0] = Rect(pos.x, pos.y, 2 * World::PIXELS_PER_CELL, World::PIXELS_PER_CELL);
                // bottom right
                result[1] = Rect(pos.x, pos.y + World::PIXELS_PER_CELL, World::PIXELS_PER_CELL, World::PIXELS_PER_CELL);
                break;
            case ORIENTATION_BOTTOM_RIGHT:
            default:
                // top
                result[0] = Rect(pos.x, pos.y, 2 * World::PIXELS_PER_CELL, World::PIXELS_PER_CELL);
                // bottom left
                result[1] = Rect(pos.x, pos.y, World::PIXELS_PER_CELL, World::PIXELS_PER_CELL);
        }
    }

};

#endif  // PLATFORM_H
