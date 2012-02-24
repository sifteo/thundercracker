#include "Anim.h"
#include "GameStateMachine.h"
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
    unsigned char mNumFrames;
    const Vec2 *mPositions;
};

struct AnimData
{
    float mDuration;
    bool mLoop;
    unsigned char mNumObjs;
    const AnimObjData *mObjs;
};

// FIXME write a tool to hide all this array/struct nesting ugliness
// FIXME reuse stuff with indexing
const static Vec2 positions[] =
{

    Vec2(2, 2),
    Vec2(8, 2),
    Vec2(7, 2),
    Vec2(6, 2),
    Vec2(5, 2),
    Vec2(4, 2),
    Vec2(3, 2),
    Vec2(2, 2),
    Vec2(2, 2),
    Vec2(2, 2),
    Vec2(2, 2),
    Vec2(3, 2),
    Vec2(4, 2),
    Vec2(5, 2),
    Vec2(6, 2),
    Vec2(7, 2),
    Vec2(8, 2),
    Vec2(8, 2),
    Vec2(8, 2),
    Vec2(8, 2),
    Vec2(7, 2),
    Vec2(6, 2),
    Vec2(5, 2),
    Vec2(4, 2),
    Vec2(3, 2),
    Vec2(2, 2),
};

const static AnimObjData animObjData[] =
{
    // AnimIndex_Tile2Idle
    { &Tile2, 0x0, 1, &positions[0]},
    { &Tile2, 0x0, 1, &positions[1]},

    // AnimIndex_Tile2SlideL
    { &Tile2, 0x0, 10, &positions[7]},
    { &Tile2, 0x0, 7, &positions[2]},

    // AnimIndex_Tile2SlideR
    { &Tile2, 0x0, 7, &positions[10]},
    { &Tile2, 0x0, 10, &positions[16]},

    // AnimIndex_Tile2Glow
    { &Tile2Glow, 0x0, 1, &positions[0]},
    { &Tile2Glow, 0x0, 1, &positions[1]},
};

const static AnimData animData[] =
{
    // AnimIndex_Tile2Idle
    { 1.f, true, 2, &animObjData[0]},

    // AnimIndex_Tile2SlideL
    { 1.f, false, 2, &animObjData[2]},

    // AnimIndex_Tile2SlideR
    { 1.f, false, 2, &animObjData[4]},

    // AnimIndex_Tile2Glow
    { 1.f, true, 2, &animObjData[6]},

};

bool animPaint(AnimIndex anim,
               VidMode_BG0_SPR_BG1 &vid,
               BG1Helper *bg1,
               float animTime,
               const AnimParams *params)
{
    const static AssetImage* fonts[] =
    {
        &Font1Letter, &Font2Letter, &Font3Letter,
    };
    const AssetImage& font = *fonts[GameStateMachine::getCurrentMaxLettersPerCube() - 1];

    const AnimData &data = animData[anim];
    float animPct =
            data.mLoop ?
                fmodf(animTime, data.mDuration)/data.mDuration :
                MIN(1.f, animTime/data.mDuration);
    const int MAX_ROWS = 16, MAX_COLS = 16;
    for (unsigned i = 0; i < data.mNumObjs; ++i)
    {
        const AnimObjData &objData = data.mObjs[i];
        unsigned char frame =
                (unsigned char) ((float)objData.mNumFrames * animPct);
        frame = MIN(frame, objData.mNumFrames - 1);
        // clip to screen
        Vec2 pos(objData.mPositions[frame]);
        // FIXME write utility AABB class
        if (pos.x >= MAX_ROWS || pos.y >= MAX_COLS)
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
        /*if (animTime > 0.f)
        {
            DEBUG_LOG(("anim time:\t%f\tpct:%f\tframe:\t%d\n", animTime, animPct, frame));
        }*/

        vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mAsset, assetFrame);
        if (params && params->mLetters && bg1)
        {
            if (i < GameStateMachine::getCurrentMaxLettersPerCube())
            {
                Vec2 letterPos(pos);
                letterPos.y += 3; // TODO
                unsigned frame = params->mLetters[i] - (int)'A';

                if (frame < font.frames)
                {
                    bg1->DrawPartialAsset(letterPos, Vec2(0,0), Vec2(size.x, font.height), font, frame);
                }
            }
        }

        switch (anim)
        {
        case AnimIndex_Tile2SlideL:
            break;

        default:
            break;
        }
    }

    // finished?
    return data.mLoop || animTime <= data.mDuration;
}


