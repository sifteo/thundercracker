#include "Anim.h"
#include "assets.gen.h"
/*
enum AnimObjIndex
{
    AnimObjIndex_Tile0,
    AnimObjIndex_Tile1,
    AnimObjIndex_Tile2,

    NumAnimObjIndexIndexes
};

enum AnimObjFlags
{
    AnimObjFlags_None,
    AnimObjFlags_Hide = (1 << 0),
};
*/

/*
struct Point
{
    int x, y;
};

struct Bla
{
    int another_var;
    Point *foo;
};


Bla test =
{
    0, (Point[]) {(Point){1, 2}, (Point){3, 4}}
};


const static Vec2 bar = Vec2(2, 8);
//Vec2 *bar = (Vec2[]){Vec2(2, 8)};

struct Bla2
{
    int another_var;
    Vec2 *foo;
};

Bla2 test2 =
{
    0, (Vec2[]) {Vec2(1, 2), Vec2(3, 4)}
};

const static AnimObjData *foo =
        (AnimObjData[]){{ 0, (Vec2[]){ Vec2(2, 8), Vec2(2, 8), Vec2(2, 8) }}};
*/

struct AnimObjData
{
    const AssetImage *mAsset;
    uint32_t mInvisibleFrames; // bitmask
    const Vec2 *mPositions;
};

struct AnimData
{
    float mDuration;
    unsigned char mNumFrames;
    unsigned char mNumObjs;
    const AnimObjData *mObjs;
};

// FIXME write a tool to hide all this array/struct nesting ugliness
const static Vec2 positions[] =
{
    Vec2(2, 2),
    Vec2(8, 2),

    Vec2(2, 2),
    Vec2(8, 2),
    Vec2(2, 2),
    Vec2(6, 2),
    Vec2(2, 2),
    Vec2(4, 2),
    Vec2(2, 2),
    Vec2(2, 2),
};

const static AnimObjData animObjData[] =
{
    { &Tile2, 0, &positions[0]},
    { &Tile2, 0, &positions[1]},

    { &Tile2, 0, &positions[0]},
    { &Tile2, 0, &positions[1]},
};

const static AnimData animData[] =
{
    // AnimIndex_2TileIdle
    { 1.f, 1, 2, &animObjData[0]},

    // AnimIndex_2TileSlideL
    { 1.f, 4, 2, &animObjData[2]},
};

void animPaint(AnimIndex anim,
               VidMode_BG0_SPR_BG1 &vid,
               BG1Helper *bg1,
               float animTime,
               const AnimParams *params)
{
    const AnimData &data = animData[anim];
    float animPct = fmodf(animTime, data.mDuration)/data.mDuration;
    unsigned char frame = (unsigned char) ((float)data.mNumFrames * animPct);
    const int MAX_ROWS = 16, MAX_COLS = 16;
    for (unsigned i = 0; i < data.mNumObjs; ++i)
    {
        const AnimObjData &objData = data.mObjs[i];
        // clip to screen
        Vec2 pos(objData.mPositions[frame]);
        // FIXME write utility AABB class
        if (pos.x >= MAX_ROWS ||
            pos.y >= MAX_COLS)
        {
            continue; // totally offscreen
        }
        pos.x = MAX(pos.x, 0);
        pos.y = MAX(pos.y, 0);
        Vec2 clipOffset = pos - objData.mPositions[frame];
        if (clipOffset.x >= (int)objData.mAsset->width ||
            clipOffset.y >= (int)objData.mAsset->height)
        {
            continue; // totally offscreen
        }
        Vec2 size(objData.mAsset->width, objData.mAsset->height);
        size.x -= abs<int>(clipOffset.x);
        size.y -= abs<int>(clipOffset.y);
        size.x = MIN(size.x, MAX_ROWS - pos.x);
        size.y = MIN(size.y, MAX_COLS - pos.y);
        ASSERT(size.x > 0);
        ASSERT(size.y > 0);
        ASSERT(objData.mAsset);
        unsigned assetFrame = 0;
        vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mAsset, assetFrame);
        switch (anim)
        {
        case AnimIndex_2TileSlideL:
            break;

        default:
            break;
        }
    }
}


