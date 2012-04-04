#ifndef THING_H
#define THING_H
#include <sifteo.h>

using namespace Sifteo;

const int PIXELS_PER_GRID = 32;
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 128;

class Thing {
public:
    int id;
    Float2 pos;
    Float2 vel;
    const PinnedAssetImage *pImage;

    Thing(int id0, Int2 pos0){
        id = id0;
        pos = pos0.toFloat();
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

        if (   pos.x < 0.0 || (pos.x + pixelWidth()) > SCREEN_WIDTH
            || pos.y < 0.0 || (pos.y + pixelHeight()) > SCREEN_HEIGHT){
            collided(NULL);
        }
    }

    void draw(VidMode_BG0_SPR_BG1 vid){
//         LOG(("Drawing thing(%d) at %.2f %.2f\n", id, pos.x, pos.y));
        vid.moveSprite(id, pos.toInt());
    }

    int pixelWidth(){
        return pImage->pixelWidth();
    }

    int pixelHeight(){
        return pImage->pixelHeight();
    }

    bool isTouching(Thing *otherThing){
        int myRight = pos.x + pixelWidth() - 1;
        int myBottom = pos.y + pixelHeight() - 1;
        int otherRight = otherThing->pos.x + otherThing->pixelWidth() - 1;
        int otherBottom = otherThing->pos.y + otherThing->pixelHeight() - 1;

        bool overlappingX = pos.x <= otherRight && myRight >= otherThing->pos.x;
        bool overlappingY = pos.y <= otherBottom && myBottom >= otherThing->pos.y;
        return overlappingX && overlappingY;
    }

    void collided(Thing *otherThing){
        vel = Vec2(0.0f, 0.0f);
        pos.x = int(pos.x / PIXELS_PER_GRID + 0.5f) * PIXELS_PER_GRID;
        pos.y = int(pos.y / PIXELS_PER_GRID + 0.5f) * PIXELS_PER_GRID;
    }

};


class Platform : public Thing {
public:

    Platform(int id0, Int2 pos0) : Thing(id0, pos0) {
    }

    virtual void think(_SYSCubeID cubeId){
        _SYSTiltState tilt = _SYS_getTilt(cubeId);
        const float TILT_ACCELERATION = 2.0;
        vel.x += (tilt.x - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
        vel.y += (tilt.y - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
    }
};


#endif // THING_H

