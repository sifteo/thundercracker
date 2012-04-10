#ifndef TURTLE_H
#define TURTLE_H

#include "thing.h"

class Turtle : public Thing {
  private :
    typedef Thing super; // Private prevents erroneous use by other classes.

  public:
    Int2 direction;

    Turtle(World &world, int id, Int2 pos) : Thing(world, id, pos){}

    bool canWalkTo(CellNum dest){
        ASSERT(isValidCellNum(dest));

        for(int i=0; i < pWorld->numThings; i++){
            if (pWorld->things[i] && pWorld->things[i]->occupiesCell(dest)){
                return true;    // There's something we can walk on
            }
        }
        
        return false; // nothing to walk on
    }


    virtual void think(_SYSCubeID cubeId){
        _SYSTiltState tilt = _SYS_getTilt(cubeId);
        const float TILT_ACCELERATION = 2.0;

        int nextCellOffset = (tilt.x - _SYS_TILT_NEUTRAL) + (tilt.y - _SYS_TILT_NEUTRAL) * World::CELL_NUM_PITCH;
        CellNum nextCell = cellNum() + nextCellOffset;

        // if applying force:
        if (nextCellOffset && canWalkTo(nextCell)){
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

};

#endif // TURTLE_H
