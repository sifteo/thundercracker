#ifndef THING_H
#define THING_H
#include <sifteo.h>
#include <string.h>
#include "rect.h"

using namespace Sifteo;


class Thing;

typedef int CellNum;

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
    int platformsMustStop;

    World(){
        numThings = 0;
        memset(things, 0, arraysize(things));
        platformsMustStop = PLATFORM_MOVE_DELAY;
    }

    void mainLoopReset(){
        platformsMustStop = MAX(0, platformsMustStop-1);
    }

    void addThing(Thing *thing){
        ASSERT(numThings < MAX_THING);
        things[numThings] = thing;
        numThings++;
    }

};

class Thing {
  public:
    static const int NUM_BOUNDS_RECTS = 2;

    World *pWorld;
    int id;
    Float2 pos;
    Float2 vel;
    const PinnedAssetImage *pImage;

    Thing(World &world, int id, Int2 pos){
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

        pos = pos + (vel * dt);

        // collision with edges
        if (   pos.x < 0.0 || (pos.x + pixelWidth())  > VidMode::LCD_width
            || pos.y < 0.0 || (pos.y + pixelHeight()) > VidMode::LCD_height){
            collided(NULL);
        }
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

    void collided(Thing *otherThing){
        vel = Vec2(0.0f, 0.0f);
        pos = nearestCellCoordinate(pos);
    }

    CellNum cellNum(){
        Int2 snapPt = nearestCellCoordinate(pos);
        int result = ((snapPt.y / World::PIXELS_PER_CELL) * World::CELL_NUM_PITCH) + (snapPt.x / World::PIXELS_PER_CELL);
        return result;
    }


    // If you can walk to 'to', then return pointer to Thing at that cell.
    // Otherwise return NULL
    Thing * canWalk(CellNum from, CellNum to){
        ASSERT(isValidCellNum(from));
        ASSERT(isValidCellNum(to));

        for(int i=0; i < World::MAX_THING; i++){
            if (pWorld->things[i] && pWorld->things[i]->cellNum() == to){
                return pWorld->things[i];
            }
        }
        
        return NULL;
    }

    bool isValidCellNum(CellNum cellNum){
        int cellX = cellNum % World::CELLS_PER_ROW;
        int cellY = int(cellNum / World::CELL_NUM_PITCH);
        return (cellX >= 0
            || cellX < World::CELLS_PER_ROW
            || cellY >= 0
            || cellY < World::CELLS_PER_ROW);
    }

    // returns the screen space x,y coordinate of the nearest cell
    Int2 nearestCellCoordinate(Float2 point){
        return Vec2( int(pos.x / World::PIXELS_PER_CELL + 0.5f) * World::PIXELS_PER_CELL,
                     int(pos.y / World::PIXELS_PER_CELL + 0.5f) * World::PIXELS_PER_CELL);
    }

};


#endif // THING_H

