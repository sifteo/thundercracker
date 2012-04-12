#ifndef PLATFORM_H
#define PLATFORM_H

#include "share.h"
#include "assets.gen.h"
#include "thing.h"

class Platform : public Thing {
  private:
    typedef Thing super;
    
  public:

    Platform(World &world, int id, Int2 pos) : Thing(world, id, pos) {} 

    virtual void think(_SYSCubeID cubeId){
        _SYSTiltState tilt = _SYS_getTilt(cubeId);
        const float TILT_ACCELERATION = 0.2;
        vel.x += (tilt.x - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
        vel.y += (tilt.y - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
        if (!isMoving && vel.len2() > 0.0001){
            isMoving = true;
            pWorld->numMovingPlatforms++;
//             LOG(("Platform %d: slide sound: start\n", id));
            channel.play(sliding_sound, LoopRepeat);
        }
    }

    virtual void onCollision(Thing *other){
        if (isMoving){
            isMoving = false;
            pWorld->numMovingPlatforms--;
//             LOG(("Platform %d: slide sound: STOP\n", id));
            channel.stop();
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
    
    LPlatform(World &world, int id, Int2 pos) : Platform(world, id, pos) {
        orientation = ORIENTATION_TOP_LEFT;
    }

    virtual bool occupiesCell(CellNum dest){
        Float2 destPoint = cellNumToPoint(dest);
        Rect theBounds[NUM_BOUNDS_RECTS];
        bool result = this->contains(destPoint);
        return result;
    }

    void setSpriteImage(VidMode_BG0_SPR_BG1 &vid, const PinnedAssetImage &asset){
        pImage = &asset;
    }

    virtual void draw(VidMode_BG0_SPR_BG1 vid){
        ASSERT(pImage->frames == 4);

        int frameNum = orientation;
        vid.setSpriteImage(id, *pImage, frameNum);
        vid.moveSprite(id, worldToScreen(pos - TOP_LEFT_OFFSET));
    }

    virtual void bounds(Rect result[NUM_BOUNDS_RECTS]){
        Float2 topLeft = pos - TOP_LEFT_OFFSET;
        switch(orientation){
            case ORIENTATION_TOP_LEFT:

                // top right
                result[0] = Rect(topLeft.x + 1,  topLeft.y, 1, 1 );
                // bottom
                result[1] = Rect(topLeft.x,     topLeft.y + 1, 2, 1);
                break;
            case ORIENTATION_TOP_RIGHT:
                // top left
                result[0] = Rect(topLeft.x,     topLeft.y, 1, 1);
                // bottom
                result[1] = Rect(topLeft.x,     topLeft.y + 1, 2, 1);
                break;
            case ORIENTATION_BOTTOM_LEFT:
                // top
                result[0] = Rect(topLeft.x,     topLeft.y, 2, 1);
                // bottom right
                result[1] = Rect(topLeft.x + 1, topLeft.y + 1, 1, 1);
                break;
            case ORIENTATION_BOTTOM_RIGHT:
            default:
                // top
                result[0] = Rect(topLeft.x,     topLeft.y, 2, 1);
                // bottom left
                result[1] = Rect(topLeft.x,     topLeft.y + 1, 1, 1);
        }
        const int MARGIN = 0.1;
        result[0].size.x -= MARGIN;
        result[0].size.y -= MARGIN;
        result[1].size.x -= MARGIN;
        result[1].size.y -= MARGIN;
    }

};

#endif  // PLATFORM_H
