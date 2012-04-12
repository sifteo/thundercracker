#ifndef TURTLE_H
#define TURTLE_H

#include "thing.h"

class Turtle : public Thing {
  private:
    typedef Thing super;

  public:
    Int2 direction;

    Turtle(World &world, int id, Int2 pos) : Thing(world, id, pos){ }

    Thing *thingAt(CellNum location){
        ASSERT(isValidCellNum(location));

        for(int i=0; i < pWorld->numThings; i++){
            if (pWorld->things[i] && pWorld->things[i]->occupiesCell(location)){
                return pWorld->things[i];
            }
        }
        
        return NULL;
    }

    bool canWalkTo(CellNum dest){
        return thingAt(dest) != NULL;
    }

    virtual void onPlatformMoved(Thing &sender, CellNum cell, Float2 movement){
        if (sender.contains(pos)){
            pos = pos + movement;
        }
    }

    virtual void think(_SYSCubeID cubeId){
        _SYSTiltState tilt = _SYS_getTilt(cubeId);

        int nextCellOffset = (tilt.x - _SYS_TILT_NEUTRAL) + (tilt.y - _SYS_TILT_NEUTRAL) * World::CELL_NUM_PITCH;
        CellNum nextCell = cellNum() + nextCellOffset;

        // if applying force:
        if (nextCellOffset && canWalkTo(nextCell)){
            if (!isMoving){
                isMoving = true;
                pWorld->numMovingTurtles++;
            }
            const float TILT_ACCELERATION = 1.0;
            vel.x = (tilt.x - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
            vel.y = (tilt.y - _SYS_TILT_NEUTRAL) * TILT_ACCELERATION;
        } else {
            // stop moving
            vel.x = 0;
            vel.y = 0;
            pos = nearestCellCoordinate(pos);
            if (isMoving){
                isMoving = false;
                pWorld->numMovingTurtles--;
            }
        }
    }

    virtual void draw(VidMode_BG0_SPR_BG1 vid){
        if (vel.len2() > World::VEL_NEAR_ZERO){
            // direction we are facing.

            enum {DOWN, RIGHT, UP, LEFT};
            int frame = 0;
            if (abs(vel.x) > abs(vel.y)){
                if (vel.x > 0) frame = RIGHT;
                else frame=LEFT;
            } else {
                if (vel.y > 0) frame = DOWN;
                else frame = UP;
            }
            setSpriteImage(vid, *pImage, frame);
        }

        super::draw(vid);
    }
};

#endif // TURTLE_H
