#ifndef TURTLE_H
#define TURTLE_H

#include "thing.h"

class Turtle : public Thing {
  private :
    typedef Thing super; // Private prevents erroneous use by other classes.

  public:
    Int2 direction;

    Turtle(World &world, int id, Int2 pos) : Thing(world, id, pos){}

    virtual void think(_SYSCubeID cubeId){
        _SYSTiltState tilt = _SYS_getTilt(cubeId);
        const float TILT_ACCELERATION = 2.0;

        int nextCellOffset = (tilt.x - _SYS_TILT_NEUTRAL) + (tilt.y - _SYS_TILT_NEUTRAL) * World::CELL_NUM_PITCH;
        int nextCell = cellNum() + nextCellOffset;

        // if no force:
        //     if nearest cell is ahead but we would hit it this turn
        //         snap to it.
        // if force:
        if (nextCellOffset){
            //     is space open in our walk direction? :
            if (canWalk(cellNum(), nextCell)){
                const float TILT_ACCELERATION = 70.0;
                vel.x = (tilt.x - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
                vel.y = (tilt.y - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
            } else {
                // stop moving
                vel.x = 0;
                vel.y = 0;
                pos = nearestCellCoordinate(pos);
            }
        }
    }

};

#endif // TURTLE_H
