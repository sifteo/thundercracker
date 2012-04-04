#ifndef THING_H
#define THING_H
#include <sifteo.h>

using namespace Sifteo;

class Thing {
public:
    int id;
    Float2 pos;
    Float2 vel;

    Thing(int id0, Int2 pos0){
        id = id0;
        pos = pos0.toFloat();
    }

    virtual void think(_SYSCubeID cubeId){
    }

    void act(float dt){
        const int MAX_VEL = 200.0;
        if (vel.len() > MAX_VEL) vel = vel.normalize() * MAX_VEL;
        vel = vel * 0.95;       // friction

        pos = pos + (vel * dt);

        while (pos.x < 0.0)   pos.x += 128.0;
        while (pos.x > 128.0) pos.x -= 128.0;

        while (pos.y < 0.0)   pos.y += 128.0;
        while (pos.y > 128.0) pos.y -= 128.0;
    }

    void draw(VidMode_BG0_SPR_BG1 vid){
//         LOG(("Drawing thing(%d) at %.2f %.2f\n", id, pos.x, pos.y));
        vid.moveSprite(id, pos.toInt());
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

