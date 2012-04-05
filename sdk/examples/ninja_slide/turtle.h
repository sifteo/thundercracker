#ifndef TURTLE_H
#define TURTLE_H

#include "thing.h"

class Turtle : public Thing {

  public:

    Turtle(World &world, int id, Int2 pos) : Thing(world, id, pos){}

    virtual void think(_SYSCubeID cubeId){
        _SYSTiltState tilt = _SYS_getTilt(cubeId);
        const float TILT_ACCELERATION = 2.0;
        vel.x += (tilt.x - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
        vel.y += (tilt.y - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
    }

};

#endif // TURTLE_H
