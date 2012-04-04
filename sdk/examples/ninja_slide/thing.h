#ifndef THING_H
#define THING_H
#include <sifteo.h>

using namespace Sifteo;

const int PIXELS_PER_GRID = 32;

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

    void draw(VidMode_BG0_SPR_BG1 vid){
        vid.moveSprite(id, pos.toInt());
    }

    int pixelWidth(){ return pImage->pixelWidth(); }
    int pixelHeight(){ return pImage->pixelHeight(); }
    int right(){ return pos.x + pixelWidth() - 1; }
    int bottom(){ return pos.y + pixelHeight() - 1; }

    bool isTouching(Thing *otherThing){
        return pos.x <= otherThing->right() && right() >= otherThing->pos.x
            && pos.y <= otherThing->bottom() && bottom() >= otherThing->pos.y;
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


#endif // THING_H

