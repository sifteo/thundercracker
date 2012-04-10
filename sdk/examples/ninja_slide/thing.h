#ifndef THING_H
#define THING_H
#include <sifteo.h>
#include "rect.h"

using namespace Sifteo;


class Thing;

// World is information that all Things have access to.
class World {
  public:
    static const int PLATFORM_MOVE_DELAY = 2;
    static const int PIXELS_PER_CELL = 32;
    static const int CELLS_PER_ROW = 4;
    static const int CELL_NUM_PITCH = CELLS_PER_ROW+1; // Add this to your cellNum to get to the next row.
        // We want an invalid cellNum at end of each row to detect when we have gone of the edge.
    static const int MAX_THING = 5;

    /*
     * properties
     */
    int numThings;
    Thing *things[MAX_THING];
    int numMovingTurtles;
    int numMovingPlatforms;
    

    World(){
        numThings = 0;
        numMovingTurtles = 0;
        numMovingPlatforms = 0;
        _SYS_memset32(reinterpret_cast<uint32_t*>(things), 0, arraysize(things));
    }

    void addThing(Thing *thing){
        ASSERT(numThings < MAX_THING);
        things[numThings] = thing;
        numThings++;
    }
};

/*
 * CellNum
 *
 *  0  1  2  3
 *  5  6  7  8
 * 10 11 12 13
 * 14 15 16 17
 */
typedef int CellNum;

bool isValidCellNum(CellNum cellNum){
    int cellX = cellNum % World::CELL_NUM_PITCH;
    int cellY = int(cellNum / World::CELL_NUM_PITCH);
    return (cellX >= 0
        || cellX < World::CELLS_PER_ROW
        || cellY >= 0
        || cellY < World::CELLS_PER_ROW);
}

Float2 cellNumToPoint(CellNum cellNum){
    int cellX = cellNum % World::CELL_NUM_PITCH;
    int cellY = int(cellNum / World::CELL_NUM_PITCH);
    return Vec2(cellX * World::PIXELS_PER_CELL, cellY * World::PIXELS_PER_CELL);
}

class Thing {
  public:
    static const int NUM_BOUNDS_RECTS = 2;

    World *pWorld;
    int id;
    Float2 pos;
    Float2 vel;
    const PinnedAssetImage *pImage;
    bool isMoving;

    Thing(World &world, int id, Int2 pos){
        this->isMoving = false;
        this->pWorld = &world;
        this->id = id;
        this->pos = pos.toFloat();
        this->pImage = NULL;
        this->pWorld->addThing(this);
    }

    void setSpriteImage(VidMode_BG0_SPR_BG1 &vid, const PinnedAssetImage &asset){
        vid.setSpriteImage(id, asset);
        pImage = &asset;
    }


    virtual void think(_SYSCubeID cubeId){
    }

    virtual void act(float dt){
//         vel = vel * 0.95;       // friction

        Float2 movement = (vel * dt);
        pos = pos + movement;

        if (movement.len2() > 0.1){
            CellNum myCell = cellNum();
            for(int i=0; i < pWorld->numThings; i++){
                if (pWorld->things[i] == this) continue;
                pWorld->things[i]->onPlatformMoved(*this, myCell, movement);
            }
        }

        // collision with edges
        if (   pos.x < 0.0 || (pos.x + pixelWidth())  > VidMode::LCD_width
            || pos.y < 0.0 || (pos.y + pixelHeight()) > VidMode::LCD_height){
            onCollision(NULL);
        }
    }

    virtual void onPlatformMoved(Thing &sender, CellNum cell, Float2 movement){
    }

    virtual void draw(VidMode_BG0_SPR_BG1 vid){
        vid.moveSprite(id, pos.toInt());
    }

    int pixelWidth(){ return pImage->pixelWidth(); }
    int pixelHeight(){ return pImage->pixelHeight(); }

    virtual void bounds(Rect result[NUM_BOUNDS_RECTS]){
        result[0] = Rect(pos, Vec2(pixelWidth(), pixelHeight()));
        result[1] = EMPTY_RECT;
    }

    bool isTouching(Thing &otherThing){
        Rect myBounds[NUM_BOUNDS_RECTS];
        Rect otherBounds[NUM_BOUNDS_RECTS];

        bounds(myBounds);
        otherThing.bounds(otherBounds);

        return myBounds[0].isTouching(otherBounds[0])
            || myBounds[0].isTouching(otherBounds[1])
            || myBounds[1].isTouching(otherBounds[0])
            || myBounds[1].isTouching(otherBounds[1]);
    }

    bool contains(Float2 point){
        Rect myBounds[NUM_BOUNDS_RECTS];
        bounds(myBounds);
        return myBounds[0].contains(point)
            || myBounds[1].contains(point);
    }

    virtual void onCollision(Thing *otherThing){
        vel = Vec2(0.0f, 0.0f);
        pos = nearestCellCoordinate(pos);
    }

    CellNum cellNum(){
        Int2 snapPt = nearestCellCoordinate(pos);
        int result = ((snapPt.y / World::PIXELS_PER_CELL) * World::CELL_NUM_PITCH) + (snapPt.x / World::PIXELS_PER_CELL);
        return result;
    }

    virtual bool occupiesCell(CellNum theCell){
        return cellNum()==theCell;
    }


    // returns the screen space x,y coordinate of the nearest cell
    Int2 nearestCellCoordinate(Float2 point){
        return Vec2( int((pos.x / World::PIXELS_PER_CELL) + 0.5f) * World::PIXELS_PER_CELL,
                     int((pos.y / World::PIXELS_PER_CELL) + 0.5f) * World::PIXELS_PER_CELL);
    }

};


#endif // THING_H

