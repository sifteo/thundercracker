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
    static const float VEL_NEAR_ZERO = 0.0001;

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
 * 15 16 17 18
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
    return Vec2((float)cellX, (float)cellY);
}

class Thing {
  public:
    static const int NUM_BOUNDS_RECTS = 2;
#define TOP_LEFT_OFFSET Vec2(0.5f, 0.5f)

    World *pWorld;
    int id;
    Float2 pos;  // in world coordinates
    Float2 vel;
    const PinnedAssetImage *pImage;
    bool isMoving;

    Thing(World &world, int id, Float2 pos){
        this->isMoving = false;
        this->pWorld = &world;
        this->id = id;
        this->pos = pos;
        this->pImage = NULL;
        this->pWorld->addThing(this);
    }

    /* Screen coordinates are 0..128, 0..128 where 0,0 is the upper left corner of the cube's LCD
     * World coordinate are aligned to the centers of the cells 
     *          E.g. world 0.0, 0.0 = 16,16 in screen coords.
     *          upper left corner 0, 0 in screen = -0.5, -0.5 in world coords
     */
    static Int2 worldToScreen(Float2 worldPt){
        Float2 screenPt = (worldPt + TOP_LEFT_OFFSET) * World::PIXELS_PER_CELL;
        return screenPt.toInt();
    }

    void setSpriteImage(VidMode_BG0_SPR_BG1 &vid, const PinnedAssetImage &asset){
        vid.setSpriteImage(id, asset);
        pImage = &asset;
    }

    void setSpriteImage(VidMode_BG0_SPR_BG1 &vid, const PinnedAssetImage &asset, int frame){
        vid.setSpriteImage(id, asset, frame);
        pImage = &asset;
    }


    virtual void think(_SYSCubeID cubeId){
    }

    virtual void act(float dt){
//         vel = vel * 0.95;       // friction

        Float2 movement = (vel * dt);
        pos += movement;

        if (movement.len2() > 0.0001){
            CellNum myCell = cellNum();
            for(int i=0; i < pWorld->numThings; i++){
                if (pWorld->things[i] == this) continue;
                pWorld->things[i]->onPlatformMoved(*this, myCell, movement);
            }
        }

        // collision with edges
        if (   (pos.x < 0.0) || (pos.x + cellWidth() > World::CELLS_PER_ROW)
            || (pos.y < 0.0) || (pos.y + cellHeight() > World::CELLS_PER_ROW)){
            onCollision(NULL);
        }
    }

    virtual void onPlatformMoved(Thing &sender, CellNum cell, Float2 movement){
    }

    virtual void draw(VidMode_BG0_SPR_BG1 vid){
        vid.moveSprite(id, worldToScreen(pos - TOP_LEFT_OFFSET));
    }

    int pixelWidth(){ ASSERT(pImage && "Thing.pImage not yet set"); return pImage->pixelWidth(); }
    int pixelHeight(){ ASSERT(pImage && "Thing.pImage not yet set"); return pImage->pixelHeight(); }
    
    int cellWidth(){ ASSERT(pImage && "Thing.pImage not yet set"); return pImage->pixelWidth() / World::PIXELS_PER_CELL; }
    int cellHeight(){ ASSERT(pImage && "Thing.pImage not yet set"); return pImage->pixelHeight() / World::PIXELS_PER_CELL; }

    virtual void bounds(Rect result[NUM_BOUNDS_RECTS]){
        const int MARGIN = 0.1;
        result[0] = Rect(pos - TOP_LEFT_OFFSET, Vec2(cellWidth() - MARGIN, cellWidth() - MARGIN));
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

    virtual void onCollision(Thing *other){
        vel = Vec2(0.0f, 0.0f);
        pos = nearestCellCoordinate(pos);
    }

    CellNum cellNum(){
        Int2 snapPt = nearestCellCoordinate(pos);
        int result = (snapPt.y * World::CELL_NUM_PITCH) + snapPt.x ;
        return result;
    }

    virtual bool occupiesCell(CellNum theCell){
        return cellNum()==theCell;
    }

    static Float2 nearestCellCoordinate(Float2 point){
        Float2 result = Vec2(float(int(point.x + 0.5f)), float(int(point.y + 0.5f)));
        return result;
    }

};


#endif // THING_H

